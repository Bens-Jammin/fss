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

void FSCrawl(string rootDir, int parentID, int& nextID, std::vector<FileEntry>& entries,
             const IgnoreRules& ignoreRules) {

    std::error_code err;
    fs::path root = rootDir;

    if (!fs::exists(root, err)) {
        if ( err && parentID == -1 ) {
            throw FSSException(
                FSS_STATUS::CrawlErr,
                "Root path does not exist or is inaccessible: " + rootDir + " (" + err.message() + ")"
            );
        }
        return; // vanished mid-crawl. skip
    }

    // Never skip the crawl root itself, even if it happens to match a
    // blacklist rule - only skip descendants.
    if (parentID != -1 && ignoreRules.shouldSkip(root)) {
        return;
    }

    bool isDir = fs::is_directory(root, err);
    if (err) {
        return; // unable to determine type. skip
    }

    std::time_t mtime;
    try {
        mtime = getMTime(root);
    } catch (const std::exception&) {
        mtime = 0;
    }

    int id = nextID++;
    entries.push_back({
        id,
        parentID,
        root.string(),
        root.filename().string(),
        isDir ? "" : root.extension().string(),
        isDir,
        mtime,
    });

    if (isDir) {
        fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, err);
        if ( err ) {
            return; // can't open. index but without children
        }

        for (; it != fs::directory_iterator(); it.increment(err) ) {
            if ( err ) {
                break; // iteration failed (entry disappeared ?) stop but keep what we have
            }

            FSCrawl(it->path().string(), id, nextID, entries, ignoreRules);
        }
    }
}

bool FSCrawl(string rootDir, std::vector<FileEntry>& entries) {
    IgnoreRules defaultRules;
    int nextID = 0;
    FSCrawl(rootDir, -1, nextID, entries, defaultRules);
    return true;
}

bool FSCrawl(string rootDir, std::vector<FileEntry>& entries, const IgnoreRules& ignoreRules) {
    int nextID = 0;
    FSCrawl(rootDir, -1, nextID, entries, ignoreRules);
    return true;
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