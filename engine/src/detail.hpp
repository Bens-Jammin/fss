#pragma once
#include <cstdlib>
#include <cstring>
#include "fss/fss.hpp"
#include "sqlite3.h"

bool DBisEmpty(sqlite3* db);
std::unordered_map<string, int64_t> getIDs(sqlite3* db);
std::unordered_map<std::string, int64_t> getMTimes(sqlite3* db);
int64_t currentFileMTime(string path);
FSS_RESULT result(FSS_STATUS status, const std::string& msg);
void updateDir(string root, std::unordered_map<string, int64_t>& mtimes, std::vector<string>& pathBuf);
