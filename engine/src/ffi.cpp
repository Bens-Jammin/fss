// C ABI (extern"C") layer exposing the indexer (init/update/query/metadata) to other languages
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

struct fss_handle {
    FSSIndexer index;
};


extern "C" fss_handle* fss_open(const char* root) noexcept {
    try {
        return new fss_handle { FSSIndexer(root) };
    } catch (...) { return nullptr; }
}


extern "C" void fss_close(fss_handle* handle) noexcept {
    delete handle; 
}

extern "C" void fss_handle_update(fss_handle* handle) noexcept {
    if (handle == nullptr) return;
    handle->index.update();   
}



extern "C" char* fss_handle_query_for(fss_handle* handle, const char* name) noexcept {
    if (handle == nullptr) return nullptr;
    try {
        auto results = handle->index.queryExact(name);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_handle_query_like(fss_handle* handle, const char* pattern) noexcept {
    if (handle == nullptr) return nullptr;
    try {
        auto results = handle->index.querySubstr(pattern);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fss_handle_query_extension(fss_handle* handle, const char* ext) noexcept {
    if (handle == nullptr) return nullptr;
    try {
        auto results = handle->index.queryExtension(ext);

        std::ostringstream oss;
        for (auto& r : results) oss << r << '\n';
        return to_owned_cstr(oss.str());
    } catch (...) {
        return nullptr;
    }
}


extern "C" char* fetch_index_metadata(fss_handle* handle) noexcept {
    if (handle == nullptr) return nullptr;
    try {
        std::unordered_map<std::string, std::string> metadata = handle->index.metadata();

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