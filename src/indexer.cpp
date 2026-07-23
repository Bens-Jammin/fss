// api to interact with the db and crawler
#include "fss.hpp"
#include <unordered_map>
#include <chrono>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;



bool DBisEmpty(sqlite3* db) {
    const char* q = "SELECT COUNT(*) FROM files;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) != SQLITE_OK) return true;
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return count == 0;
}

FSS_RESULT result(FSS_STATUS status, const std::string& msg) {
    char* buf = static_cast<char*>(std::malloc(msg.size() + 1));
    std::memcpy(buf, msg.c_str(), msg.size() + 1);
    return { status, buf };
}

FSSIndexer::FSSIndexer() : root{TEST_ROOT_DIRECTORY}, dbPath{DBPath(root)}, debug{false} {
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

FSSIndexer::FSSIndexer(string root) : root{root}, dbPath{DBPath(root)}, debug{false} {
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

        FSCrawl(this->root, files);
        insertFileEntries(files, this->dbPath);
        update_metadata_table(this->root);
    } catch (const FSSException& e) {
        return result(e.status, e.what());
    } catch (const std::exception& e) {
        return result(FSS_STATUS::OtherErr, e.what());
    } catch (...) {
        return result(FSS_STATUS::OtherErr, "Unknown error during update");
    }
    return result( FSS_STATUS::Ok, "" );
}


std::unordered_map<string, int64_t> getIDs(sqlite3* db) {
    const char* query = "SELECT path, id FROM files WHERE is_dir = TRUE";

    sqlite3_stmt* statement;
    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        return {};
    }

    std::unordered_map<std::string, int64_t> results;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char* path = sqlite3_column_text(statement, 0);
        int64_t id = sqlite3_column_int64(statement, 1);
        if (path) {
            results.emplace(reinterpret_cast<const char*>(path), id);
        }
    }
    sqlite3_finalize(statement);
    return results;
}



/// @brief a helper function to fetch the last stored times of modification of all directories in the database
/// @param db the sqlite database
/// @return a map from the file path to the modify time
std::unordered_map<std::string, int64_t> getMTimes(sqlite3* db) {
    const char* query = "SELECT path, mtime FROM files WHERE is_dir = TRUE";

    sqlite3_stmt* statement;
    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        return {};
    }

    std::unordered_map<std::string, int64_t> results;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const unsigned char* path = sqlite3_column_text(statement, 0);
        int64_t mtime = sqlite3_column_int64(statement, 1);
        if (path) {
            results.emplace(reinterpret_cast<const char*>(path), mtime);
        }
    }
    sqlite3_finalize(statement);
    return results;
}


/// @brief fs interface to get the last modification time for a path
/// @param path path to file or dir
int64_t currentFileMTime(string path) {
    
    auto ftime = fs::last_write_time( fs::path( path ) );
    int64_t currentMTime = std::chrono::system_clock::to_time_t(
        std::chrono::clock_cast<std::chrono::system_clock>(ftime)
    );   
    return currentMTime;
} 


void updateDir(string root, std::unordered_map<string, int64_t>& mtimes, std::vector<string>& pathBuf) {

    if ( !fs::exists(root) || !fs::is_directory(root) )
        return;

    for (const auto& entry : fs::directory_iterator( root )) {
        string entryPath = entry.path().string();

        if (fs::is_regular_file(entry.status())) {
            pathBuf.push_back( entryPath );
        } else {
            pathBuf.push_back( entryPath );
            // if the subdir has been updated, recursively run this function on it
            int64_t currentMTime = currentFileMTime( entryPath );
            auto it = mtimes.find(entryPath);
            if (it == mtimes.end() || it->second != currentMTime) {
                updateDir(entryPath, mtimes, pathBuf);
            } 
        }
    }
}


/// @brief reindexes the tree
/// @attention This needs to be updated at some point! Not efficient
FSS_RESULT FSSIndexer::update() {

    if (!fs::exists(this->root)) {
        return result(FSS_STATUS::CrawlErr, "Root path does not exist: " + this->root);
    }
 
    sqlite3* db = nullptr;

    try {
        db = openDB(this->dbPath);
        if (db == nullptr) {
            return result( FSS_STATUS::DbOpen, "Unable to open DB" );
        }

        std::unordered_map<string, int64_t> mtimes = getMTimes(db);
        
        int64_t currentRootMTime = currentFileMTime( this->root );
        auto it = mtimes.find(this->root);
        if (it != mtimes.end() && it->second == currentRootMTime) {
            sqlite3_close(db);
            return result(FSS_STATUS::Ok, ""); // no change
        }
    
        std::vector<string> pathBuffer;
        updateDir(root, mtimes, pathBuffer);
    
        // We also need each entry's parent id, since files table is a self-referencing
        // tree via parent_id. Build a path->id lookup once, up front, rather than
        // querying per-file inside the loop below.
        std::unordered_map<string, int64_t> pathToId = getIDs(db);
    
        std::vector<FileEntry> entries;
        entries.reserve(pathBuffer.size());
    
        for (const auto& path : pathBuffer) {
            fs::path p(path);

            std::error_code err;

            bool isDir = fs::is_directory(p, err);
            if (err) {
                continue; // entry vanished/became inaccessible. skip
            }
    
            FileEntry fe;
            fe.path      = path;
            fe.filename  = p.filename().string();
            fe.extension = p.has_extension() ? p.extension().string() : "";
            fe.isDir     = isDir;
            fe.mtime     = currentFileMTime(path);
    
            string parentPath = p.parent_path().string();
            auto parentIt = pathToId.find(parentPath);
            fe.parentID = (parentIt != pathToId.end()) ? parentIt->second : -1;
    
            entries.push_back(std::move(fe));
        }
    
        updateEntries(this->dbPath, entries);
        update_metadata_table(this->root);
        sqlite3_close(db);
        return result(FSS_STATUS::Ok, "");
    } catch (const FSSException& e) {
        if (db) sqlite3_close(db);
        return result(e.status, e.what());
    } catch (const std::exception& e) {
        if (db) sqlite3_close(db);
        return result(FSS_STATUS::OtherErr, e.what());
    } catch (...) {
        if (db) sqlite3_close(db);
        return result(FSS_STATUS::OtherErr, "Unknown error during update");
    }

}



std::vector<string> FSSIndexer::queryExtension(const char* name) { 

    sqlite3* db = openDB(this->dbPath);
    if (db == nullptr) {
        std::cerr << "Unable to open DB file.\n";
        return {""};
    }

    const char* query = "SELECT path FROM files WHERE extension = ?";

    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return {""};
    }

    sqlite3_bind_text(statement, 1, name, -1, SQLITE_TRANSIENT);

    std::vector<string> results;
    while (sqlite3_step(statement) == SQLITE_ROW) { // get ALL results, not just the first
        const unsigned char* path = sqlite3_column_text(statement, 0);
        if (path) results.push_back(reinterpret_cast<const char*>(path));
    }

    if (results.empty()) {
        if (this->debug) std::cerr << "No match found for filename: " << name << "\n";
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}

std::vector<string> FSSIndexer::queryFor(const char* name) { 

    sqlite3* db = openDB(this->dbPath);
    if (db == nullptr) {
        if (this->debug) std::cerr << "Unable to open DB file.\n";
        return {""};
    }

    const char* query = "SELECT path FROM files WHERE filename = ? COLLATE NOCASE;";

    sqlite3_stmt* statement;


    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        if (this->debug) std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return {""};
    }

    sqlite3_bind_text(statement, 1, name, -1, SQLITE_TRANSIENT);

    std::vector<string> results;
    while (sqlite3_step(statement) == SQLITE_ROW) { // get ALL results, not just the first
        const unsigned char* path = sqlite3_column_text(statement, 0);
        if (path) results.push_back(reinterpret_cast<const char*>(path));
    }

    if (results.empty()) {
        if (this->debug) std::cerr << "No match found for filename: " << name << "\n";
    }


    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}

std::vector<string> FSSIndexer::queryLike(const char* name) {
    
    sqlite3* db = openDB(this->dbPath);
    if (db == nullptr) {
        if (this->debug) std::cerr << "Unable to open DB file.\n";
        return {""};
    }

    const char* query = "SELECT path FROM files WHERE filename LIKE ? COLLATE NOCASE;";

    sqlite3_stmt* statement;


    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        if (this->debug) std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return {""};
    }

    string pattern = string("%") + name + "%";  // adds wildcards to not fuck up the query
    sqlite3_bind_text(statement, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<string> results;
    while (sqlite3_step(statement) == SQLITE_ROW) { // get ALL results, not just the first
        const unsigned char* path = sqlite3_column_text(statement, 0);
        if (path) results.push_back(reinterpret_cast<const char*>(path));
    }

    if (results.empty()) {
        if (this->debug) std::cerr << "No match found for filename: " << name << "\n";
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}

std::vector<string> FSSIndexer::queryFuzzy(string name) { return {""}; }
