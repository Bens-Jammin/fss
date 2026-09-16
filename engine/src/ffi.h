// Foreign Function Interface
#pragma once
extern "C" {

    void fss_init(const char* root) noexcept;
    void fss_update(const char* root) noexcept; 

    char* fss_query_for(const char* root, const char* name) noexcept;
    char* fss_query_like(const char* root, const char* pattern) noexcept;
    char* fss_query_extension(const char* root, const char* ext) noexcept;
    char* fetch_index_metadata(const char* root) noexcept;
    void fss_free(char* str) noexcept;
}