// crypto.cpp
// --------------------------------
// Implementation file for crypto functions.
// Credits: aggeloskwn7 (github)
// --------------------------------
#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>

namespace {
    constexpr size_t KEY_LEN = 32;
    constexpr size_t TAG_LEN = 16;
}

// everything below is pretty self explanatory, you just need to know how to use OpenSSL EVP API. For any help please contact me through Github. I will be happy to help.

bool derive_key_pbkdf2(
    const std::string& master_password,
    const std::vector<uint8_t>& salt,
    uint32_t iterations,
    std::vector<uint8_t>& out_key
) {
    out_key.resize(KEY_LEN);
    const EVP_MD* md = EVP_sha256();
    int ok = PKCS5_PBKDF2_HMAC(
        master_password.data(),
        static_cast<int>(master_password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        static_cast<int>(iterations),
        md,
        static_cast<int>(out_key.size()),
        out_key.data()
    );
    return ok == 1;
}

bool aes256gcm_encrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& aad,
    std::vector<uint8_t>& ciphertext,
    std::vector<uint8_t>& tag
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;
    bool success = false;
    int len = 0;

    do {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) break;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) break;
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) break;

        if (!aad.empty()) {
            if (EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), static_cast<int>(aad.size())) != 1) break;
        }

        ciphertext.resize(plaintext.size());
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), static_cast<int>(plaintext.size())) != 1) break;
        int ciphertext_len = len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) break;
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);

        tag.resize(TAG_LEN);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data()) != 1) break;

        success = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    return success;
}

bool aes256gcm_decrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& aad,
    const std::vector<uint8_t>& tag,
    std::vector<uint8_t>& plaintext
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;
    bool success = false;
    int len = 0;

    do {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) break;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) break;
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) break;

        if (!aad.empty()) {
            if (EVP_DecryptUpdate(ctx, nullptr, &len, aad.data(), static_cast<int>(aad.size())) != 1) break;
        }

        plaintext.resize(ciphertext.size());
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) break;
        int plaintext_len = len;

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), const_cast<uint8_t*>(tag.data())) != 1) break;

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            // auth failed (wrong password/tag mismatch)
            break;
        }
        plaintext_len += len;
        plaintext.resize(plaintext_len);
        success = true;
    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    return success;
}
