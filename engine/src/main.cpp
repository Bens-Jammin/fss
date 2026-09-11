#include "fss.hpp"




int main() {
    std::cout << "hello from main!\n";
    
    FSSIndexer indexer = FSSIndexer("C:/Users/benem");
    // auto results = indexer.queryExtension(".cpp");
    // for (auto i : results) {
    //     std::cout << i << "\n";
    // }


    // std::cout << "==== metadata ====\n";
    // for (const auto& [key, value] : indexer.metadata()) {
    //     std::cout << key << ": " << value << "\n";
    // }

    indexer.done();
    return 0;
}
