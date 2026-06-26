#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>


using string = std::string;

const string TEST_ROOT_DIRECTORY = "C:/Users/benem/LocalProjects/";


void crawl(string rootDir, bool debug);