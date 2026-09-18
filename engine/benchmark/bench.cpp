#include "fss/fss.hpp"
#include <chrono>
#include <iostream>

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

int main(int argc, char** argv) {
    string root = (argc > 1) ? argv[1] : "C:/Users/benem";
    FSSIndexer indexer = FSSIndexer(root);
    time_update_speed(&indexer);
    indexer.done();
    return 0;
}