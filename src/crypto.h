// crypto.hpp
// --------------------------------
// Header file for crypto functions.
// Credits: aggeloskwn7 (github)
// --------------------------------
#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct EncBlob {
    std::vector<uint8_t> salt;       // 16B
    uint32_t iterations;             // e.g., 200'000
    std::vector<uint8_t> iv;         // 12B
    std::vector<uint8_t> ciphertext; // len = plaintext + padding?
    std::vector<uint8_t> tag;        // 16B
};

bool derive_key_pbkdf2(
    const std::string& master_password,
    const std::vector<uint8_t>& salt,
    uint32_t iterations,
    std::vector<uint8_t>& out_key // 32B
);

bool aes256gcm_encrypt(
    const std::vector<uint8_t>& key,     // 32B
    const std::vector<uint8_t>& iv,      // 12B
    const std::vector<uint8_t>& plaintext,
    const std::vector<uint8_t>& aad,     // can be empty
    std::vector<uint8_t>& ciphertext,
    std::vector<uint8_t>& tag            // 16B
);

bool aes256gcm_decrypt(
    const std::vector<uint8_t>& key,     // 32B
    const std::vector<uint8_t>& iv,      // 12B
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& aad,
    const std::vector<uint8_t>& tag,     // 16B
    std::vector<uint8_t>& plaintext
);
