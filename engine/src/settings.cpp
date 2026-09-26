#include "settings.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

void register_index(string root) {

    // app(end) to the file
    std::ofstream index_file(INDEX_REGISTER_PATH, std::ios::app);

    if (!index_file.is_open()) {
        std::cerr << "FATAL: Could not open index register!" << std::endl;
    }

    fs::path absolute_root = fs::absolute( fs::path(root) );

    index_file << root << std::endl;
}


void clear_index_from_register(string root);
void registerd_indices() {
    std::ifstream index_file(INDEX_REGISTER_PATH);
    if (!index_file.is_open()) {
        std::cerr << "FATAL: Could not open index register!" << std::endl;
    }

    std::stringstream buff;
    buff << index_file.rdbuf();


    std::vector<string> index_list;
    string line;

    int i = 0;
    // Read line-by-line from the stringstream into the vector
    while (std::getline(buff, line)) {
        index_list.push_back(line);
        std::cout << "- " << line << std::endl;
        i++;
    }
    std::cout << i << " total indexes registered." << std::endl;
}