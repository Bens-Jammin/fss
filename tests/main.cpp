#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/fss.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

const fs::path TEST_DIRECTORY = fs::absolute( fs::path("../") );

TEST_CASE("indexer finds files by extension") {
    FSSIndexer indexer = FSSIndexer();
    indexer.build_index();

    CHECK(indexer.queryExtension(".h").size() == 2);
    CHECK(indexer.queryExtension(".md").size() == 1);
    CHECK(indexer.queryExtension(".js").empty());

    indexer.done();
}

TEST_CASE("indexer finds files by exact name") {
    
    FSSIndexer indexer = FSSIndexer();
    indexer.build_index();
    
    CHECK(indexer.queryFor("doctest.h").size() == 1);
    CHECK(indexer.queryFor("main.cpp").size() == 2);
    CHECK(indexer.queryFor("Makefile").size() == 1);
    CHECK(indexer.queryFor("main.js").empty());
    
    indexer.done();
}
    
    
TEST_CASE("indexers on different root stay independent") {
        
    fs::path rootA = fs::temp_directory_path() / "fss_test_root1";
    fs::path rootB = fs::temp_directory_path() / "fss_test_root2";

    fs::remove_all(rootA);
    fs::remove_all(rootB);
    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream out(rootA / "only_in_a.rootA_marker");
        out << "belongs to root A\n";
    }
    {
        std::ofstream out(rootB / "only_in_b.rootB_marker");
        out << "belongs to root B\n";
    }

    FSSIndexer indexerA(rootA.string());
    indexerA.build_index();

    FSSIndexer indexerB(rootB.string());
    indexerB.build_index();


    // Sanity: they resolved to different DB files
    CHECK(DBPath(rootA.string()) != DBPath(rootB.string()));

    // Each indexer finds its own file...
    CHECK(indexerA.queryExtension(".rootA_marker").size() == 1);
    CHECK(indexerB.queryExtension(".rootB_marker").size() == 1);

    // ...and does NOT see the other root's file
    CHECK(indexerA.queryExtension(".rootB_marker").empty());
    CHECK(indexerB.queryExtension(".rootA_marker").empty());


    // Cleanup
    fs::remove_all(rootA);
    fs::remove_all(rootB);
    indexerA.done();
    indexerB.done();
}