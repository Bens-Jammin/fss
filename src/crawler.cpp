#include "fss.hpp"
using string = std::string;
namespace fs = std::filesystem;



/// @brief scans through the file tree to determine the number of elements
/// @param rootDir 
/// @return the number of total fs entries crawled (directories and files)
int scan(string rootDir, int count=0, bool debug=false) {

    fs::path root = rootDir;

    if ( !fs::exists(root) ) {
        std::cerr << "[FSS ERROR] CRAWLER --" << rootDir << " doesnt exist.\n";
        return count;
    }

    if (!fs::is_directory(root) ) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER -- found file '" << rootDir << "'\n";
        return ++count;
    }

    int dirCount = 0;
    for (const auto& entry : fs::directory_iterator(root) ) {
        string entryName = entry.path().string();
        dirCount += scan(entryName, count, debug);
    }
    if (debug) std::cout << "[FSS DEBUG] CRAWLER -- Dir '" << rootDir << "' had " << dirCount << " entries.\n";
    return dirCount;
}

// api method
int scan(string rootDir, bool debug) {
    return scan(rootDir, 0, debug);
}

void FSCrawl(string rootDir, int parentID, int& nextID, std::vector<FileEntry>& entries, bool debug) {
    fs::path root = rootDir;
    if (!fs::exists(root)) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER ERROR --" << rootDir << " doesnt exist.\n";
        return;
    }

    int id = nextID++;
    bool isDir = fs::is_directory(root);
    entries.push_back({
        id,
        parentID,
        root.string(),
        root.filename().string(),
        isDir ? "" : root.extension().string(),
        isDir,
        getMTime(root)
    });

    if (isDir) {
        for (const auto& entry : fs::directory_iterator(root)) {
            string entryName = entry.path().string();
            FSCrawl(entryName, id, nextID, entries, debug);
        }
    }
}

void FSCrawl(string rootDir, std::vector<FileEntry>& entries, bool debug) {
    int nextID = 0;
    FSCrawl(rootDir, -1, nextID, entries, debug);
}



void crawl(string rootDir, bool debug) {

    fs::path root = rootDir;

    if ( !fs::exists(root) ) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER ERROR --" << rootDir << " doesnt exist.\n";
        return;
    }

    if (!fs::is_directory(root) ) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER MESSAGE -- found file '" << rootDir << "'\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(root) ) {
        string entryName = entry.path().string();
        crawl(entryName, debug);
    }

}