#include "ffi.h"
#include "fss.hpp"
#include <cstring>
#include <cstdlib>
#include <sstream>


extern "C" char* fss_query_for(const char* name) {
    FSSIndexer indexer = FSSIndexer();
    indexer.update();   // TEMP!
    auto results = indexer.queryFor(name);

    std::ostringstream oss;
    for (auto& r : results ) oss << r << '\n';
    std::string joinedResults = oss.str();
    
    char* output = static_cast<char*>( std::malloc(joinedResults.size() + 1) );
    if (!output) return nullptr;
    std::memcpy(output, joinedResults.c_str(), joinedResults.size() + 1);
    return output;
}

extern "C" void fss_free(char* str) {
    std::free(str);
}