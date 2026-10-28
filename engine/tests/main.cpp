#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/fss/fss.hpp"
#include "../src/db.hpp"
#include "../src/ffi.h"
#include "../src/utils.hpp"
#include "../src/settings.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>
#include "doctest.h"
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;


const string TEST_DIRECTORY = fs::absolute( fs::path("../") ).string();

long long queryInt(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    long long val = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return val;
}

std::string queryText(sqlite3* db, const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    std::string val = text ? reinterpret_cast<const char*>(text) : "";
    sqlite3_finalize(stmt);
    return val;
}


TEST_CASE("indexer finds files by extension") {
    FSSIndexer indexer = FSSIndexer(TEST_DIRECTORY);
    indexer.build_index();

    CHECK(indexer.queryExtension(".cpp").size() > 5);
    CHECK(indexer.queryExtension(".md").size() == 1);
    CHECK(indexer.queryExtension(".js").empty());

    indexer.done();
}

TEST_CASE("indexer finds files by exact name") {
    
    FSSIndexer indexer = FSSIndexer(TEST_DIRECTORY);
    indexer.build_index();
    
    CHECK(indexer.queryExact("doctest.h").size() == 1);
    CHECK(indexer.queryExact("main.cpp").size() == 3);
    CHECK(indexer.queryExact("Makefile").size() == 2);
    CHECK(indexer.queryExact("main.js").empty());
    
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


TEST_CASE("indexer handles a nonexistent root") {
    fs::path badRoot = fs::temp_directory_path() / "fss_test_does_not_exist_xyz";
    fs::remove_all(badRoot); // make sure it really doesn't exist

    CHECK_THROWS_AS(FSSIndexer indexer(badRoot.string()), FSSException);
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

    size_t countAfterFirst = indexer.queryExtension(".txt").size();

    // run again immediately, nothing on disk changed
    FSS_RESULT second = indexer.update();
    CHECK(second.status == FSS_STATUS::Ok);

    
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

    // FSSIndexer indexer(root.string());
    CHECK_THROWS_AS(FSSIndexer(root.string()), FSSException);

    fs::remove_all(root);
}


TEST_CASE("ffi handle reuses one index and queries never update it") {
    fs::path root = fs::temp_directory_path() / "fss_test_ffi_root";
    fs::remove_all(root);
    fs::create_directories(root);
    { std::ofstream out(root / "first.marker"); out << "one\n"; }

    fss_handle* handle = fss_open(root.string().c_str());
    REQUIRE(handle != nullptr);

    auto query = [&](void) {
        char* raw = fss_handle_query_extension(handle, ".marker");
        REQUIRE(raw != nullptr);
        std::string results(raw);
        fss_free(raw);
        return results;
    };

    // opening a cold index builds it, so the first query already sees the file
    CHECK(query().find("first.marker") != std::string::npos);

    // a file added after opening is invisible until an explicit update: the
    // query must neither rebuild nor error on the now-stale index
    { std::ofstream out(root / "second.marker"); out << "two\n"; }
    CHECK(query().find("second.marker") == std::string::npos);

    // an explicit update is allowed and does not disturb the handle
    fss_handle_update(handle);
    CHECK(query().find("first.marker") != std::string::npos);

    fss_close(handle);
    fs::remove_all(root);
}

TEST_CASE("check a metadata table is created when an index is made") {
    fs::path root = fs::temp_directory_path() / "root";
    fs::create_directories(root);
    { std::ofstream out(root / "file1.txt"); out << "hello\n"; }

    FSSIndexer indexer = FSSIndexer(root.string());
    indexer.build_index();

    std::string dbPath = DBPath(root.string());
    sqlite3* db = openDB(dbPath);

    auto row_count = queryInt(db, "SELECT COUNT(*) FROM index_metadata");
    CHECK(row_count == 1);

    auto root_val = queryText(db, "SELECT root FROM index_metadata WHERE id = 1");
    CHECK(root_val == root);

    auto last_update = queryInt(db, "SELECT last_update FROM index_metadata WHERE id = 1");
    CHECK(last_update > 0);

    sqlite3_close(db);
    fs::remove_all(root);
    indexer.done(); 
}


TEST_CASE("same normalized path (lowercase) is not registered more than once") {
    fs::path root = fs::temp_directory_path() / "fss_test_reg_root";
    fs::remove_all(root);
    fs::create_directories(root);
    { std::ofstream out(root / "file.txt"); out << "hello\n"; }

    // Create indexer twice with different case variations
    {
        FSSIndexer indexer1(root.string());
    }
    {
        std::string rootUpper = root.string();
        for (char& c : rootUpper) {
            if (std::isalpha((unsigned char)c)) c = std::toupper((unsigned char)c);
        }
        FSSIndexer indexer2(rootUpper);
        indexer2.done();
    }

    // Count occurrences of the normalized path in the register
    std::ifstream reg(INDEX_REGISTER_PATH);
    REQUIRE(reg.is_open());
    std::string line;
    int count = 0;
    fs::path absRoot = fs::absolute(root).lexically_normal();
    std::string normalized = absRoot.string();
    for (char& c : normalized) {
        if ((unsigned char)c < 128 && std::isalpha((unsigned char)c)) c = std::tolower((unsigned char)c);
    }
    while (std::getline(reg, line)) {
        std::string lowerLine = line;
        // trim whitespace
        while (!lowerLine.empty() && (lowerLine.back() == '\r' || lowerLine.back() == '\n')) lowerLine.pop_back();
        for (char& c : lowerLine) {
            if ((unsigned char)c < 128 && std::isalpha((unsigned char)c)) c = std::tolower((unsigned char)c);
        }
        if (lowerLine == normalized) {
            count++;
        }
    }
    CHECK(count == 1);

    // cleanup
    try { fs::remove(DBPath(root.string())); } catch (...) {}
    fs::remove_all(root);
}