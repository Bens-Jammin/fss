// api to interact with the db and crawler
#include <unordered_map>
#include <chrono>
#include <vector>
#include <cstring>
#include "fss/fss.hpp"
#include "detail.hpp"
#include "utils.hpp"
#include "db.hpp"
#include "crawler.hpp"
#include "settings.hpp"

namespace fs = std::filesystem;


FSSIndexer::FSSIndexer(string root) : root{root}, dbPath{DBPath(root)}, debug{false} {
    bool existed = DBExists(dbPath);
    if (!existed) initDB(root, dbPath);

    sqlite3* db = openDB(dbPath);
    bool needsBuild = !existed || DBisEmpty(db);
    sqlite3_close(db);

    register_index(root);

    if (needsBuild) {
        FSS_RESULT res = this->build_index();
        if (res.status != FSS_STATUS::Ok) {
            throw FSSException(res.status, res.message);
        }
    }
}

FSSIndexer::FSSIndexer(string root, bool debug) : root{root}, dbPath{DBPath(root)}, debug{debug} {
    bool existed = DBExists(dbPath);
    if (!existed) initDB(root, dbPath);

    sqlite3* db = openDB(dbPath);
    bool needsBuild = !existed || DBisEmpty(db);
    sqlite3_close(db);

    if (needsBuild) {
        FSS_RESULT res = this->build_index();
        if (res.status != FSS_STATUS::Ok) {
            throw FSSException(res.status, res.message);
        }
    }
}


/// @brief cleanup all artifacts relating to the index. **PERMANENTLY DELETES THE DATABASE!!**
void FSSIndexer::done() {
    clearDB(this->root);
}


FSS_RESULT FSSIndexer::build_index() {

    if (!fs::exists(this->root)) {
        return result(FSS_STATUS::CrawlErr, "Root path does not exist: " + this->root);
    }
    

    std::vector<FileEntry> files;
    try {

        using clock = std::chrono::steady_clock;

        fs::path root = this->root;
        auto t0 = clock::now();
        FSCrawl(root, files);
        auto t1 = clock::now();

        insertFileEntries(files, this->dbPath);
        auto t2 = clock::now();

        update_metadata_table(this->root);
        auto t3 = clock::now();

        auto ms = [](auto start, auto end) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        };

        auto fstime = ms(t0, t1);
        auto inserttime = ms(t1, t2);
        auto metatime = ms(t2,t3);
        auto totaltime = ms(t0,t3);

        std::cerr << "FSCrawl:                " << fstime     << " ms (" << (fstime     * 100)/totaltime << "%)\n" ;
        std::cerr << "insertFileEntries:      " << inserttime << " ms (" << (inserttime * 100)/totaltime << "%)\n" ;
        std::cerr << "update_metadata_table:  " << metatime   << " ms (" << (metatime   * 100)/totaltime << "%)\n" ;
        std::cerr << "Total:                  " << totaltime  << " ms (" << (totaltime  * 100)/totaltime << "%)\n" ;
    
    } catch (const FSSException& e) {
        return result(e.status, e.what());
    } catch (const std::exception& e) {
        return result(FSS_STATUS::OtherErr, e.what());
    } catch (...) {
        return result(FSS_STATUS::OtherErr, "Unknown error during update");
    }
    return result( FSS_STATUS::Ok, "" );
}


std::unordered_map<string, string> FSSIndexer::metadata() {
    return fetch_metadata_for(this->root);
}