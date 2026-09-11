#include "fss.hpp"
#include <chrono>

using string = std::string;
namespace fs = std::filesystem;


void FSCrawl(fs::directory_entry node, int parentID, int& nextID, std::vector<FileEntry>& entries) {
    
    std::error_code err;
    const fs::path& root = node.path();

    fs::file_status status = node.status(err); // file metadata

    if (err) {
        if (parentID == -1) {
            throw FSSException(
                FSS_STATUS::CrawlErr,
                "Root path does not exist or is inaccessible: " + root.string() + " (" + err.message() + ")"
            );
        }
        return; // vanished mid-crawl, skip
    }
    if (!fs::exists(status)) {
        if (parentID == -1) {
            throw FSSException(FSS_STATUS::CrawlErr, "Root path does not exist: " + root.string());
        }
        return;
    }


    bool isDir = fs::is_directory(root, err);
        
    std::error_code mtimeErr;
    auto ftime = node.last_write_time(mtimeErr);
    std::time_t mtime = 0;
    if (!mtimeErr) {
        auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
        mtime = std::chrono::system_clock::to_time_t(sctp);
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

            FSCrawl(*it, id, nextID, entries);
        }
    }
}

bool FSCrawl(fs::path rootDir, std::vector<FileEntry>& entries) {
    int nextID = 0;
    std::error_code err;
    fs::directory_entry rootEntry(rootDir, err);
    
    if (err) {
        throw FSSException(
            FSS_STATUS::CrawlErr,
            "Root path does not exist or is inaccessible: " + rootDir.string() + " (" + err.message() + ")"
        );
    }
    
    FSCrawl(rootEntry, -1, nextID, entries);
    return true;
}