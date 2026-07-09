#include "fss.hpp"




int main() {
    std::cout << "hello from main!\n";
    
    FSSIndexer indexer = FSSIndexer(TEST_ROOT_DIRECTORY);
    auto results = indexer.queryExtension(".cpp");
    for (auto i : results) {
        std::cout << i << "\n";
    }

    indexer.update();

    results = indexer.queryExtension(".cpp");
    for (auto i : results) {
        std::cout << i << "\n";
    }


    indexer.done();
    return 0;
}
