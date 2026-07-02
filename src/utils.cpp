#include <filesystem>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;


std::time_t getMTime(const fs::path& p) {
    auto ftime = fs::last_write_time(p);

    // Convert file_time_type -> system_clock::time_point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );

    return std::chrono::system_clock::to_time_t(sctp);
}