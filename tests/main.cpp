#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../src/fss.hpp"
#include "../src/exception.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>
#include "doctest.h"
#include <iostream>

namespace fs = std::filesystem;

const fs::path TEST_DIRECTORY = fs::path(__FILE__).parent_path() / "..";


TEST_CASE ("debug: show where the test directory is") {
    std::cout << "TEST DIR: " << TEST_DIRECTORY << "\n";
}

TEST_CASE("indexer finds files by extension") {
    FSSIndexer indexer = FSSIndexer();
    indexer.build_index();

    CHECK(indexer.queryExtension(".cpp").size() > 5);
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


TEST_CASE("indexer handles a nonexistent root gracefully") {
    fs::path badRoot = fs::temp_directory_path() / "fss_test_does_not_exist_xyz";
    fs::remove_all(badRoot); // make sure it really doesn't exist

    FSSIndexer indexer(badRoot.string());
    FSS_RESULT r = indexer.build_index();
    std::cout << r.describe() << "\n";

    CHECK(r.status != FSS_STATUS::Ok);
    REQUIRE(r.message != nullptr);
    CHECK(std::string(r.message).find("not exist") != std::string::npos);

    // and it shouldn't have crashed or left a half-built index behind
    CHECK(indexer.queryFor("anything").empty());

    free(r.message);
    indexer.done();
}

TEST_CASE("indexer skips permission-denied subdirectories without failing the whole update") {
    fs::path root = fs::temp_directory_path() / "fss_test_perm_root";
    fs::path lockedDir = root / "locked";
    fs::path lockedFile = lockedDir / "secret.txt";
    fs::path visibleFile = root / "visible.marker";

    fs::remove_all(root);
    fs::create_directories(lockedDir);
    { std::ofstream out(lockedFile); out << "should not be indexed\n"; }
    { std::ofstream out(visibleFile); out << "should be indexed\n"; }

    fs::permissions(lockedDir, fs::perms::none, fs::perm_options::replace);

    FSSIndexer indexer(root.string());
    FSS_RESULT r = indexer.build_index();

    // overall update should still succeed even though one subdir was unreadable
    CHECK(r.status == FSS_STATUS::Ok);

    // the file we could reach is indexed...
    CHECK(indexer.queryExtension(".marker").size() == 1);
    
    if (r.message) free(r.message);

    // restore perms so cleanup can actually delete the directory
    fs::permissions(lockedDir, fs::perms::owner_all, fs::perm_options::replace);
    indexer.done();
    fs::remove_all(root);
}

TEST_CASE("update is a no-op when nothing changed since last index") {
    fs::path root = fs::temp_directory_path() / "fss_test_noop_root";
    fs::remove_all(root);
    fs::create_directories(root);
    { std::ofstream out(root / "file1.txt"); out << "hello\n"; }

    FSSIndexer indexer(root.string());
    FSS_RESULT first = indexer.build_index();
    CHECK(first.status == FSS_STATUS::Ok);
    if (first.message) free(first.message);

    size_t countAfterFirst = indexer.queryExtension(".txt").size();

    // run again immediately, nothing on disk changed
    FSS_RESULT second = indexer.update();
    CHECK(second.status == FSS_STATUS::Ok);
    if (second.message) free(second.message);

    // index contents should be identical, not duplicated or emptied
    CHECK(indexer.queryExtension(".txt").size() == countAfterFirst);

    indexer.done();
    fs::remove_all(root);
}


TEST_CASE("update recovers from a corrupted/non-sqlite file at the db path") {
    fs::path root = fs::temp_directory_path() / "fss_test_corrupt_root";
    fs::remove_all(root);
    fs::create_directories(root);
    { std::ofstream out(root / "file.txt"); out << "hello\n"; }

    fs::path dbPath = fs::path(DBPath(root.string()));
    fs::create_directories(dbPath.parent_path());
    { std::ofstream bogus(dbPath); bogus << "not a real sqlite file"; }

    FSSIndexer indexer(root.string());
    FSS_RESULT r = indexer.build_index();

    // sqlite should refuse to treat this as a valid db — should surface
    // as an error rather than crashing or silently overwriting it unexpectedly
    CHECK(r.status != FSS_STATUS::Ok);
    if (r.message) free(r.message);

    indexer.done();
    fs::remove_all(root);
}