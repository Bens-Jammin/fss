
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
#include "exception.hpp"


using string = std::string;
namespace fs = std::filesystem;

const string TEST_ROOT_DIRECTORY = "C:\\Users\\benem\\LocalProjects\\fss";

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
        string dbPath;
        bool debug;
    public:
        FSSIndexer();
        FSSIndexer(string root);
        FSSIndexer(string root, bool debug);
        FSS_RESULT build_index();
        FSS_RESULT update();
        void done();
        std::vector<string> queryExtension(const char* name);
        std::vector<string> queryFor(const char* name);
        std::vector<string> queryLike(const char* name);
        std::vector<string> queryFuzzy(string name);
};


void clearDB(string root);
bool DBExists(string DBPath);
sqlite3* openDB(string DBPath);
void execSQL(sqlite3* db, const char* command);
int scan(string rootDir, bool debug);
bool FSCrawl(string rootDir, std::vector<FileEntry>& entries);
void crawl(string rootDir, bool debug);
void initDB(string DBPath);
void insertFileEntries(const std::vector<FileEntry>& files, string DBPath);
void updateEntries(string DBPath, const std::vector<FileEntry>& entries);
std::time_t getMTime(const std::filesystem::path& p);
string DBPath(string root);
std::int64_t epoch_now();