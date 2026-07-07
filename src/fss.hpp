
// *******************************************************************************************
// NOTE! At some point may want to split into multiple components, each with their own header
// *******************************************************************************************

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

struct ChildEntry {
    int64_t id;
    bool is_dir;
    int64_t mtime;
};

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
        bool debug;
    public:
        FSSIndexer();
        FSSIndexer(string root);
        FSSIndexer(string root, bool debug);
        void build_index();
        void update();
        std::vector<string> queryExtension(const char* name);
        std::vector<string> queryFor(const char* name);
        std::vector<string> queryLike(const char* name);
        std::vector<string> queryFuzzy(string name);
};


constexpr const char* SQLITE_DATABASE_PATH = "fss_index.db";  // must be const char* for sqlite3_open(...)

bool DBExists();
void updateEntries(const std::vector<FileEntry>& entries);
sqlite3* openDB();
int execSQL(sqlite3* db, const char* command);
int scan(string rootDir, bool debug);
void FSCrawl(string rootDir, std::vector<FileEntry>& entries, bool debug=true);
void crawl(string rootDir, bool debug);
void initDB();
void insertFileEntries(const std::vector<FileEntry>& files);
std::time_t getMTime(const std::filesystem::path& p);