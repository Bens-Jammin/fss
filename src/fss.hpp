#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "stdlib.h"
#include <sqlite3.h>


using string = std::string;
namespace fs = std::filesystem;

const string TEST_ROOT_DIRECTORY = "C:/Users/benem/LocalProjects/fss";


struct FileEntry {
    int id;
    int parentID;
    string path;
    string filename;
    string extension;
    bool isDir;
    std::time_t mtime;
};


class FSSIndexer {
    private:
        string root;
    public:
        FSSIndexer(string root=TEST_ROOT_DIRECTORY);
        void build_index();
        std::vector<string> queryExtension(const char* name);
        std::vector<string> queryFor(const char* name);
        std::vector<string> queryLike(const char* name);
        std::vector<string> queryFuzzy(string name);
};


constexpr const char* SQLITE_DATABASE_PATH = "fss_index.db";  // must be const char* for sqlite3_open(...)

bool DBExists();
sqlite3* openDB();
int scan(string rootDir, bool debug);
void FSCrawl(string rootDir, std::vector<FileEntry>& entries, bool debug=true);
void crawl(string rootDir, bool debug);
void initDB();
void insertFileEntries(const std::vector<FileEntry>& files);
std::time_t getMTime(const std::filesystem::path& p);