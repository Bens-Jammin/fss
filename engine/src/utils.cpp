#include <filesystem>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdint>

namespace fs = std::filesystem;




std::string DBPath(std::string root) {
    if (root.ends_with(".db")) {
        std::cerr << "FAIL! Cannot make an index tree for a database file (root = " << root << ")\n";
    }
    size_t hash = std::hash<std::string>{}(root);
    fs::path databasesPath = fs::absolute( fs::path("../../databases") );
    fs::create_directories(databasesPath);
    std::string path =  (databasesPath / ("fss_" + std::to_string(hash) + ".db")).string();
    return path;
}


std::int64_t epoch_now() {
    // 1. Get the current point in time
    auto now = std::chrono::system_clock::now();
    
    // 2. Extract duration since Jan 1, 1970
    auto duration = now.time_since_epoch();
    
    // 3. Cast duration specifically to seconds and extract the integer value
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}