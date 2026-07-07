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
    CHECK(indexer.queryExtension(".db").size() == 1);
    CHECK(indexer.queryExtension(".md").size() == 1);
    CHECK(indexer.queryExtension(".rs").empty());
}

TEST_CASE("indexer finds files by exact name") {
    FSSIndexer indexer = FSSIndexer();
    // indexer.build_index();

    CHECK(indexer.queryFor("doctest.h").size() == 1);
    CHECK(indexer.queryFor("main.cpp.h").size() == 2);
    CHECK(indexer.queryFor("Makefile").size() == 1);
    CHECK(indexer.queryFor("main.rs").empty());
}

TEST_CASE("update() picks up a newly added file") {
    // NOTE: adjust this path to wherever your test fixture tree actually lives
    fs::path testRoot = fs::path(TEST_DIRECTORY);
    fs::path newFile = testRoot / "incremental_test_file.rs";

    // Ensure clean slate in case a previous failed run left this behind
    if (fs::exists(newFile)) {
        fs::remove(newFile);
    }

    FSSIndexer indexer = FSSIndexer();
    // indexer.build_index();

    // Baseline: no .rs files exist yet
    CHECK(indexer.queryExtension(".rs").empty());

    // Create a new file after the initial index was built
    {
        std::ofstream out(newFile);
        out << "// test file for incremental update\n";
    }

    // Some filesystems have coarse mtime resolution (1s on some setups);
    // sleep briefly to guarantee the new file's/parent dir's mtime
    // is observably different from the baseline.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    indexer.update();

    auto results = indexer.queryExtension(".rs");
    CHECK(results.size() == 1);

    // Clean up so this test is repeatable
    fs::remove(newFile);
}

TEST_CASE("update() picks up a deleted file") {
    fs::path testRoot = fs::path(TEST_DIRECTORY);
    fs::path tempFile = testRoot / "incremental_delete_test.tmp";
    {
        std::ofstream out(tempFile);
        out << "temporary\n";
    }
    
    CHECK( fs::exists(tempFile) );
    FSSIndexer indexer = FSSIndexer();
    
    
    CHECK(indexer.queryFor("incremental_delete_test.tmp").size() == 1);

    fs::remove(tempFile);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    indexer.update();
    std::cout << "If youre seeing this this is after indexer update!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    CHECK(indexer.queryFor("incremental_delete_test.tmp").empty());
}