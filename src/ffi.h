// Foreign Function Interface
#pragma once
extern "C" {
    
    char* fss_query_for(const char* name);
    void fss_free(char* str);
}