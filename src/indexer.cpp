// api to interact with the db and crawler
#include "fss.hpp"

FSSIndexer::FSSIndexer(string root) : root{root} {
    if ( DBExists() ) return;
    initDB();
}

void FSSIndexer::build_index() {
    std::vector<FileEntry> files;
    FSCrawl(this->root, files, false);

    insertFileEntries(files);
}


std::vector<string> FSSIndexer::queryExtension(const char* name) { 

    sqlite3* db = openDB();
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
        std::cerr << "No match found for filename: " << name << "\n";
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}

std::vector<string> FSSIndexer::queryFor(const char* name) { 

    sqlite3* db = openDB();
    if (db == nullptr) {
        std::cerr << "Unable to open DB file.\n";
        return {""};
    }

    const char* query = "SELECT path FROM files WHERE filename = ? COLLATE NOCASE;";

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
        std::cerr << "No match found for filename: " << name << "\n";
    }


    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}

std::vector<string> FSSIndexer::queryLike(const char* name) {
    
    sqlite3* db = openDB();
    if (db == nullptr) {
        std::cerr << "Unable to open DB file.\n";
        return {""};
    }

    const char* query = "SELECT path FROM files WHERE filename LIKE ? COLLATE NOCASE;";

    sqlite3_stmt* statement;


    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
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
        std::cerr << "No match found for filename: " << name << "\n";
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
    return results;
}


std::vector<string> FSSIndexer::queryFuzzy(string name) { return {""}; }
