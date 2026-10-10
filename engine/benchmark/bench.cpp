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

void time_query_speed(FSSIndexer* indexer) {
    using clock = std::chrono::steady_clock;

    const char* probe = "cpp";

    auto t0 = clock::now();
    auto exact = indexer->queryExact(probe);
    auto t1 = clock::now();
    auto substr = indexer->querySubstr(probe);
    auto t2 = clock::now();
    auto ext = indexer->queryExtension(".cpp");
    auto t3 = clock::now();

    auto ms = [](auto start, auto end) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    std::cout << "queryExact(" << probe << ") time:     " << ms(t0, t1) << " ms (" << exact.size() << " hits)\n";
    std::cout << "querySubstr(" << probe << ") time:    " << ms(t1, t2) << " ms (" << substr.size() << " hits)\n";
    std::cout << "queryExtension(.cpp) time:  " << ms(t2, t3) << " ms (" << ext.size() << " hits)\n";
}

int main(int argc, char** argv) {
    string root = (argc > 1) ? argv[1] : "C:/Users/benem";
    FSSIndexer indexer = FSSIndexer(root);
    time_update_speed(&indexer);
    time_query_speed(&indexer);
    indexer.done();
    return 0;
}