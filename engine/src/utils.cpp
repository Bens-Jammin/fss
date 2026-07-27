#include <filesystem>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdint>

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
    fs::path databasesPath = fs::path(__FILE__).parent_path() / "../../databases";
    databasesPath = fs::absolute(databasesPath).lexically_normal();
    fs::create_directories(databasesPath);
    std::string path =  (databasesPath / ("fss_" + std::to_string(hash) + ".db")).string();
    return path;
}


std::string configPath(std::string root) {

    fs::path rootPath = fs::path(root);
    std::string rootBaseName = rootPath.filename().string();
    std::string rootParent = rootPath.parent_path().filename().string();

    // hash generation
    size_t hash = std::hash<std::string>{}(root) % 0x1000000; // limit to 6 hex digits (0x000000–0xFFFFFF)
    std::ostringstream hexStream;
    hexStream << std::hex << std::setw(6) << std::setfill('0') << hash;
    std::string hashStr = hexStream.str();

    // dir creation
    fs::path configPath = fs::path(__FILE__).parent_path() / "../../config";
    configPath = fs::absolute(configPath).lexically_normal();
    fs::create_directory(configPath);

    std::string path = (configPath / (rootParent + "_" + rootBaseName + "_" + hashStr + ".conf")).string();
    return path;
}