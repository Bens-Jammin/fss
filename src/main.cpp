#include "stdlib.h"
#include "fss.hpp"



void build_index() {

    initDB();

    std::vector<FileEntry> files;
    FSCrawl(TEST_ROOT_DIRECTORY, files);

    insertFileEntries(files);
}

int main() {

    // build_index();
    auto matches = queryLike("fss");
    for (const auto& path : matches) {
        std::cout << path << "\n";
    }
}
