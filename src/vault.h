// vault.hpp
// --------------------------------
// Header file for vault-related structures and functions.
// Credits: aggeloskwn7 (github)
// --------------------------------
#pragma once
#include <string>
#include <vector>

struct Entry {
    std::string website;
    std::string username;
    std::string password; // kept in memory only after unlock
};

struct Vault {
    std::vector<Entry> entries;
    bool dirty = false;
};

// save and load functions
bool save_vault(const Vault& v, const std::string& path, const std::string& master, uint32_t iterations = 200000);
bool load_vault(Vault& v, const std::string& path, const std::string& master);
