#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "doctest.h"
#include "../src/fss.hpp"

TEST_CASE("indexer finds files by extension") {

    FSSIndexer indexer = FSSIndexer();
    indexer.build_index();

    CHECK(indexer.queryExtension(".db").size() == 1);
    CHECK(indexer.queryExtension(".md").size() == 1);
    CHECK(indexer.queryExtension(".rs").empty());
}


TEST_CASE("indexer finds files by exact name") {

    FSSIndexer indexer = FSSIndexer();
    indexer.build_index();

    CHECK(indexer.queryFor("doctest.h").size() == 1);
    CHECK(indexer.queryFor("main.cpp.h").size() == 2);
    CHECK(indexer.queryFor("Makefile").size() == 1);
    CHECK(indexer.queryFor("main.rs").empty());

}
