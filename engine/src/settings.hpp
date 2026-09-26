#pragma once
#include <filesystem>
#include <string>
#include "fss/fss.hpp"

namespace fs = std::filesystem;

inline const fs::path INDEX_REGISTER_PATH = [] {
#ifdef _WIN32
    if (const char* p = std::getenv("LOCALAPPDATA")) return fs::path(p) / "fss";
    if (const char* p = std::getenv("APPDATA"))     return fs::path(p) / "fss";
#elif defined(__APPLE__)
    return fs::path(std::getenv("HOME")) / "Library" / "Application Support" / "fss";
#else
    if (const char* p = std::getenv("XDG_DATA_HOME")) return fs::path(p) / "fss";
    return fs::path(std::getenv("HOME")) / ".local" / "share" / "fss";
#endif
    return fs::current_path();
}();



void register_index(string root);
void clear_index_from_register(string root);
void registerd_indices();