#include "fss.hpp"

        
        
        

void time_update_speed(FSSIndexer* indexer) {
    using clock = std::chrono::steady_clock;

    auto t0 = clock::now();
    indexer->update();
    auto t1 = clock::now();

    
    auto ms = [](auto start, auto end) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };


    std::cout << "Update time: " << ms(t0, t1) << " ms.\n";
}

int main() {
    std::cout << "hello from main!\n";
    
    FSSIndexer indexer = FSSIndexer("C:/Users/benem");
    // auto results = indexer.queryExtension(".cpp");
    // for (auto i : results) {
    //     std::cout << i << "\n";
    // }

    time_update_speed( &indexer );

    // std::cout << "==== metadata ====\n";
    // for (const auto& [key, value] : indexer.metadata()) {
    //     std::cout << key << ": " << value << "\n";
    // }

    indexer.done();
    return 0;
}
