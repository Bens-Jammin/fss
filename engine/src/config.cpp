// config.cpp
#include "config.hpp"

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace {

const std::unordered_set<std::string> DEFAULT_BASENAME_BLACKLIST = {
    ".git", "build", "target", ".venv", "__pycache__",
};
const std::unordered_set<std::string> DEFAULT_ABS_PATH_BLACKLIST = {};

// Trims leading/trailing whitespace; returns empty string for
// whitespace-only input.
std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

}  // namespace

IgnoreRules::IgnoreRules() {
    basenames_ = DEFAULT_BASENAME_BLACKLIST;
    absolutePaths_ = DEFAULT_ABS_PATH_BLACKLIST;
}

void IgnoreRules::loadFromFile(const std::filesystem::path& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        // No user config yet - defaults stand. Not an error.
        return;
    }

    enum class Section { None, Basename, AbsPath };
    Section section = Section::None;

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string line = trim(rawLine);

        if (line.empty() || line.front() == '#') {
            continue;
        }

        if (line == "[basename]") {
            section = Section::Basename;
            continue;
        }
        if (line == "[abspath]") {
            section = Section::AbsPath;
            continue;
        }

        switch (section) {
            case Section::Basename:
                basenames_.insert(line);
                break;
            case Section::AbsPath:
                absolutePaths_.insert(
                    std::filesystem::path(line).lexically_normal().string());
                break;
            case Section::None:
                // Entry before any section header - ignore silently.
                break;
        }
    }
}


void IgnoreRules::loadOrCreate(const std::filesystem::path& configPath) {
    std::filesystem::create_directory("../../config");
    if (!std::filesystem::exists(configPath)) {
        writeDefaultConfigFile(configPath);
        return;  // in-memory defaults from the constructor already apply
    }
    loadFromFile(configPath);
}

void IgnoreRules::writeDefaultConfigFile(const std::filesystem::path& configPath) {
    std::error_code ec;
    std::filesystem::create_directories(configPath.parent_path(), ec);
    if (ec) {
        return;  // best effort - in-memory defaults still apply this run
    }

    std::ofstream file(configPath);
    if (!file.is_open()) {
        return;
    }

    file << "# fss ignore rules\n";
    file << "# [basename] - match any file/dir with this exact name, anywhere in the tree\n";
    file << "# [abspath]  - match this exact absolute path only\n\n";

    file << "[basename]\n";
    for (const auto& name : DEFAULT_BASENAME_BLACKLIST) {
        file << name << "\n";
    }

    file << "\n[abspath]\n";
    for (const auto& path : DEFAULT_ABS_PATH_BLACKLIST) {
        file << path << "\n";
    }
}


bool IgnoreRules::shouldSkip(const std::filesystem::path& entryPath) const {
    if (basenames_.count(entryPath.filename().string()) > 0) {
        return true;
    }

    const std::string normalized =
        entryPath.lexically_normal().string();
    return absolutePaths_.count(normalized) > 0;
}

void IgnoreRules::addBasename(std::string name) {
    basenames_.insert(std::move(name));
}

void IgnoreRules::addAbsolutePath(std::filesystem::path path) {
    absolutePaths_.insert(path.lexically_normal().string());
}

std::filesystem::path IgnoreRules::defaultConfigPath() {

}

const std::unordered_set<std::string>& IgnoreRules::defaultBasenames() {
    return DEFAULT_BASENAME_BLACKLIST;
}

const std::unordered_set<std::string>& IgnoreRules::defaultAbsolutePaths() {
    return DEFAULT_ABS_PATH_BLACKLIST;
}