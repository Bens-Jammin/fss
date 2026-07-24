// Foreign Function Interface
#pragma once
extern "C" {
    
    char* fss_query_for(const char* name) noexcept;
    char* fss_query_like(const char* pattern) noexcept;
    char* fss_query_extension(const char* ext) noexcept;
    char* fetch_index_metadata() noexcept;
    void fss_free(char* str) noexcept;
}