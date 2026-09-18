#pragma once

#include "types.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

using string = std::string;


class FSSIndexer {
    private:
        string root;
        string dbPath;
        bool debug;
    public:
        FSSIndexer(string root);
        FSSIndexer(string root, bool debug);
        FSS_RESULT build_index();
        FSS_RESULT update();
        void done();
        std::unordered_map<string, string> metadata();
        std::vector<string> queryExtension(const char* name);
        std::vector<string> queryExact(const char* name);
        std::vector<string> querySubstr(const char* name);
        std::vector<string> queryFuzzy(string name);
};