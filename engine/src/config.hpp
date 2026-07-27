// config.hpp
#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

class IgnoreRules {
public:
    // Seeds with hardcoded repo defaults (see config.cpp).
    IgnoreRules();

    // Loads and merges rules from a config file. Missing file is not an
    // error - defaults simply remain in effect. Malformed lines are skipped.
    void loadFromFile(const std::filesystem::path& configPath);

    // If configPath exists, loads it (merging with in-memory defaults).
    // If it doesn't exist, writes it out populated with the current
    // defaults so the user has something to edit later.
    void loadOrCreate(const std::filesystem::path& configPath);

    // True if this entry should be skipped (and, for directories, its
    // entire subtree not recursed into).
    bool shouldSkip(const std::filesystem::path& entryPath) const;

    void addBasename(std::string name);
    void addAbsolutePath(std::filesystem::path path);

    // Resolves $XDG_CONFIG_HOME/fss/ignore.conf, falling back to
    // ~/.config/fss/ignore.conf (via $USERPROFILE on Windows/MSYS2).
    static std::filesystem::path defaultConfigPath();

    static const std::unordered_set<std::string>& defaultBasenames();
    static const std::unordered_set<std::string>& defaultAbsolutePaths();

private:
    std::unordered_set<std::string> basenames_;
    std::unordered_set<std::string> absolutePaths_;

    static void writeDefaultConfigFile(const std::filesystem::path& configPath);
};