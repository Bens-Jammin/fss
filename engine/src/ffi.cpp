#include "ffi.h"
#include "fss.hpp"
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


extern "C" char* fss_query_for(const char* name) noexcept {
    try {
        FSSIndexer indexer = FSSIndexer();
        indexer.update();   // TEMP!
        auto results = indexer.queryFor(name);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_query_like(const char* pattern) noexcept {
    try {
        FSSIndexer indexer = FSSIndexer();
        indexer.update();   // TEMP!
        auto results = indexer.queryLike(pattern);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_query_extension(const char* ext) noexcept {
    try {
        FSSIndexer indexer = FSSIndexer();
        indexer.update();   // TEMP!
        auto results = indexer.queryExtension(ext);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fetch_index_metadata(/* const char* root */) noexcept {
    try {
        FSSIndexer indexer = FSSIndexer();
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