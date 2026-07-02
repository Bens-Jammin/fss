#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>


using string = std::string;


struct FileEntry {
    int id;
    int parentID;
    string path;
    string filename;
    string extension;
    bool isDir;
    std::time_t mtime;
};

const int SQLITE_INSERT_BATCH_SIZE = 25;
const string TEST_ROOT_DIRECTORY = "C:/Users/benem/LocalProjects/";
constexpr const char* SQLITE_DATABASE_PATH = "fss_index.db";  // must be const char* for sqlite3_open(...)


int scan(string rootDir, bool debug);
void FSCrawl(string rootDir, std::vector<FileEntry>& entries, bool debug=true);
void crawl(string rootDir, bool debug);
void initDB();
void insertFileEntries(const std::vector<FileEntry>& files);
std::vector<string> queryFor(const char* name);
std::vector<string> queryLike(const char* name);
std::vector<string> queryExtension(const char* name);

std::time_t getMTime(const std::filesystem::path& p);
