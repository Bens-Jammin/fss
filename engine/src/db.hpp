#pragma once
#include "fss/fss.hpp"
#include <sqlite3.h>
#include <vector>


void initDB(string root, string DBPath);
void clearDB(string root);
bool DBExists(string DBPath);
sqlite3* openDB(string DBPath);
void execSQL(sqlite3* db, const char* command);
int execSQLWithSTDOUT(sqlite3* db, const char* command);

void insertFileEntries(const std::vector<FileEntry>& files, string DBPath);
void updateEntries(string DBPath, const std::vector<FileEntry>& entries);
void update_metadata_table(string root);
std::unordered_map<string, string> fetch_metadata_for(string root);