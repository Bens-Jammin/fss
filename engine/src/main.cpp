#include "fss/fss.hpp"
#include "settings.hpp"
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
    registerd_indices();

    indexer.done();
    return 0;
}