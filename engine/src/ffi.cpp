#include "ffi.h"
#include "fss/fss.hpp"
#include <cstring>
#include <cstdlib>
#include <sstream>

namespace {

char* to_owned_cstr(const std::string& s) {
    char* output = static_cast<char*>(std::malloc(s.size() + 1));
    if (!output) return nullptr;
    std::memcpy(output, s.c_str(), s.size() + 1);
    return output;
}

} // namespace


extern "C" void fss_init(const char* root) noexcept {
    FSSIndexer indexer(root);
}


extern "C" void fss_update(const char* root) noexcept {
    FSSIndexer indexer(root);
    indexer.update();
}


extern "C" char* fss_query_for(const char* root, const char* name) noexcept {
    try {
        FSSIndexer indexer(root);
        indexer.update();   // TEMP!
        auto results = indexer.queryExact(name);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_query_like(const char* root, const char* pattern) noexcept {
    try {
        FSSIndexer indexer(root);
        indexer.update();   // TEMP!
        auto results = indexer.querySubstr(pattern);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_query_extension(const char* root, const char* ext) noexcept {
    try {
        FSSIndexer indexer(root);
        indexer.update();   // TEMP!
        auto results = indexer.queryExtension(ext);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fetch_index_metadata(const char* root) noexcept {
    try {
        FSSIndexer indexer(root);
        std::unordered_map<std::string, std::string> metadata = indexer.metadata();

        std::ostringstream oss;
        for (const auto& [key, value] : metadata) {
            oss << key << '=' << value << '\n';
        }
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" void fss_free(char* str) noexcept {
    std::free(str);
}