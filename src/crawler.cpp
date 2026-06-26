#include "fss.hpp"
using string = std::string;
namespace fs = std::filesystem;




void crawl(string rootDir, bool debug=true) {

    fs::path root = rootDir;

    if ( !fs::exists(root) ) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER ERROR --" << rootDir << " doesnt exist.\n";
        return;
    }

    if (!fs::is_directory(root) ) {
        if (debug) std::cout << "[FSS DEBUG] CRAWLER MESSAGE -- found file '" << rootDir << "'\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(root) ) {
        string entryName = entry.path().string();
        crawl_recursive(entryName, debug);
    }

}