#include <filesystem>
#include <chrono>
#include <ctime>
#include <iostream>

namespace fs = std::filesystem;


std::time_t getMTime(const fs::path& p) {
    auto ftime = fs::last_write_time(p);

    // Convert file_time_type -> system_clock::time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sctp);
}


std::string DBPath(std::string root) {
    if (root.ends_with(".db")) {
        std::cerr << "FAIL! Cannot make an index tree for a database file (root = " << root << ")\n";
    }
    size_t hash = std::hash<std::string>{}(root);
    fs::path databasesPath = fs::absolute( fs::path("./databases") );
    fs::create_directories(databasesPath);
    std::string path =  (databasesPath / ("fss_" + std::to_string(hash) + ".db")).string();
        std::cout << "index on root " << root << "has hash " << hash << "\n";
    return path;
}