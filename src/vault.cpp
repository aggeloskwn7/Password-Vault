// vault.cpp
// --------------------------------
// Vault persistence functions.
// Credits: aggeloskwn7 (github)
// --------------------------------

#include "vault.h"
#include "crypto.h"
#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <fstream>
#include <cstring>

using json = nlohmann::json;

static const uint8_t MAGIC[4] = { 'P','M','V','1' };

// helper
static std::vector<uint8_t> to_bytes(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

// Save vault to disk (encrypted) steps explained below
bool save_vault(const Vault& v, const std::string& path, const std::string& master, uint32_t iterations) {
    // 1) Serialize entries to JSON
    json j = json::array();
    for (auto& e : v.entries) {
        j.push_back({
            {"website", e.website},
            {"username", e.username},
            {"password", e.password}
            });
    }
    auto plain = to_bytes(j.dump());

    // 2) Create salt + iv
    EncBlob blob;
    blob.salt.resize(16);
    blob.iv.resize(12);
    RAND_bytes(blob.salt.data(), (int)blob.salt.size());
    RAND_bytes(blob.iv.data(), (int)blob.iv.size());
    blob.iterations = iterations;

    // 3) Derive key + encrypt
    std::vector<uint8_t> key;
    if (!derive_key_pbkdf2(master, blob.salt, iterations, key)) return false;
    std::vector<uint8_t> aad;
    if (!aes256gcm_encrypt(key, blob.iv, plain, aad, blob.ciphertext, blob.tag)) return false;

    // 4) Write file
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    f.write((const char*)MAGIC, 4);
    f.write((const char*)blob.salt.data(), (std::streamsize)blob.salt.size());
    f.write((const char*)&blob.iterations, sizeof(blob.iterations));
    f.write((const char*)blob.iv.data(), (std::streamsize)blob.iv.size());
    f.write((const char*)blob.ciphertext.data(), (std::streamsize)blob.ciphertext.size());
    f.write((const char*)blob.tag.data(), (std::streamsize)blob.tag.size());

    return true;
}

// Load vault from disk (decrypt) steps explained below
bool load_vault(Vault& v, const std::string& path, const std::string& master) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    // 1) Check magic
    uint8_t magic[4];
    f.read((char*)magic, 4);
    if (std::memcmp(magic, MAGIC, 4) != 0) return false;

    // 2) Read salt, iterations, iv
    EncBlob b;
    b.salt.resize(16); b.iv.resize(12); b.tag.resize(16);
    f.read((char*)b.salt.data(), 16);
    f.read((char*)&b.iterations, 4);
    f.read((char*)b.iv.data(), 12);

    // 3) Read rest (ciphertext + tag)
    std::vector<uint8_t> rest((std::istreambuf_iterator<char>(f)), {});
    if (rest.size() < 16) return false;
    b.tag.assign(rest.end() - 16, rest.end());
    b.ciphertext.assign(rest.begin(), rest.end() - 16);

    // 4) Derive key + decrypt
    std::vector<uint8_t> key, plain;
    if (!derive_key_pbkdf2(master, b.salt, b.iterations, key)) return false;
    std::vector<uint8_t> aad;
    if (!aes256gcm_decrypt(key, b.iv, b.ciphertext, aad, b.tag, plain)) return false;

    // 5) Parse JSON
    auto s = std::string(plain.begin(), plain.end());
    auto j = json::parse(s, nullptr, false);
    if (j.is_discarded()) return false;

    v.entries.clear();
    for (auto& it : j) {
        v.entries.push_back(Entry{
            it.value("website",""),
            it.value("username",""),
            it.value("password","")
            });
    }
    v.dirty = false;
    return true;
}
