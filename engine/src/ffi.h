// Foreign Function Interface
#pragma once
extern "C" {
    
    char* fss_query_for(const char* name);
    char* fss_query_like(const char* pattern);
    char* fss_query_extension(const char* ext);
    void fss_free(char* str);
}