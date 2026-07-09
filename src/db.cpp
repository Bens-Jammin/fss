#include <sqlite3.h>
#include <iostream>
#include <string>
#include "fss.hpp"



bool DBExists(string DBPath) { 
    std::filesystem::path dbPath = DBPath;
    return std::filesystem::exists(dbPath); 
}


void clearDB(string root) {
    string path = DBPath(root);
    std::cout << "clearing " << path << "\n";
    fs::remove(path);   
}


static int printCallback(void* /*unused*/, int argc, char** argv, char** colNames) {
    for (int i = 0; i < argc; i++) {
        std::cout << colNames[i] << " = " << (argv[i] ? argv[i] : "NULL") << "  ";
    }
    std::cout << "\n";
    return 0;
}

/// @brief  mostly for debugging, acts as a way to interact with the db and printing results to stdout (cli ?)
/// @param db 
/// @param command 
/// @return 
int execSQLWithSTDOUT(sqlite3* db, const char* command) {
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, command, printCallback, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLITE error: " << errorMsg << "\n";
        sqlite3_free(errorMsg);
    }
    return rc;
}

int execSQL(sqlite3* db, const char* command) {
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, command, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLITE error: " << errorMsg << "\n";
        sqlite3_free(errorMsg);
    }
    return rc;
}

sqlite3* openDB(string DBPath) { 
    sqlite3* db;
    int returncode = sqlite3_open(DBPath.c_str(), &db);

    if (returncode != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return nullptr;
    }
    return db;
}


void initDB(string DBPath) {

    sqlite3* db = openDB(DBPath);
    if (db == nullptr) {
        std::cerr << "Failed to open database file.\n";
        return;
    }

    const char* create_index_table = 
        "CREATE TABLE IF NOT EXISTS files ("
        "   id         INTEGER PRIMARY KEY,"           // primary key
        "   path       TEXT NOT NULL UNIQUE,"          // full path
        "   filename   TEXT NOT NULL,"                 // for name only searches
        "   extension  TEXT,"                          // searches for file extensions
        "   parent_id  INTEGER REFERENCES files(id),"  // self-referencing tree for partial path searches
        "   is_dir     INTEGER NOT NULL DEFAULT 0,"
        "   mtime      INTEGER NOT NULL"                // unix timestamp for change detection
        ")";

        
    const char* create_indexes =
        "CREATE INDEX IF NOT EXISTS idx_files_filename ON files(filename); "
        "CREATE INDEX IF NOT EXISTS idx_files_parent ON files(parent_id); "
        "CREATE INDEX IF NOT EXISTS idx_files_extension ON files(extension);";

    int rc = execSQL(db, create_index_table);
    if (rc != SQLITE_OK) {
        std::cerr << "Could not initialize database." << "\n";
        sqlite3_close(db);
        return;
    } else { std::cout << "successfully created table.\n"; }
    
    rc = execSQL(db, create_indexes);
    if (rc != SQLITE_OK) {
        std::cerr << "Could not initialize database." << "\n";
        sqlite3_close(db);
        return;
    } else { std::cout << "successfully indexed table.\n"; }

    sqlite3_close(db);
}


void insertFileEntries(const std::vector<FileEntry>& files, string DBPath) {
    const char* insert_sql =
        "INSERT INTO files (id, path, filename, extension, parent_id, is_dir, mtime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    
    sqlite3* db = openDB(DBPath);
    if (db == nullptr) {
        std::cerr << "Unable to open DB file.\n";
        return;
    }
    
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr);

    execSQL(db, "BEGIN TRANSACTION;");

    for (const auto& f : files) {
        sqlite3_bind_int(stmt, 1, f.id);
        sqlite3_bind_text(stmt, 2, f.path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, f.filename.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, f.extension.c_str(), -1, SQLITE_TRANSIENT);

        if (f.parentID == -1)  // root has no parent
            sqlite3_bind_null(stmt, 5);
        else
            sqlite3_bind_int(stmt, 5, f.parentID);
    
        sqlite3_bind_int(stmt, 6, f.isDir ? 1 : 0);
        sqlite3_bind_int64(stmt, 7, static_cast<sqlite3_int64>(f.mtime));

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Insert failed: " << sqlite3_errmsg(db)  << " for DB at " << DBPath << "\n";
        }
        sqlite3_reset(stmt);
    }

    execSQL(db, "COMMIT;");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void updateEntries(string DBPath, const std::vector<FileEntry>& entries) {
    if (entries.empty()) return;

    sqlite3* db = openDB( DBPath );
    if (db == nullptr) {
        std::cerr << "Unable to open DB file.\n";
        return;
    }

    const char* query =
        "UPDATE files "
        "SET filename = ?, extension = ?, parent_id = ?, is_dir = ?, mtime = ? "
        "WHERE path = ?";

    sqlite3_stmt* statement;
    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return;
    }

    if (sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(statement);
        sqlite3_close(db);
        return;
    }

    for (const auto& entry : entries) {
        sqlite3_bind_text(statement, 1, entry.filename.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, entry.extension.c_str(), -1, SQLITE_TRANSIENT);

        if (entry.parentID != -1) {
            sqlite3_bind_int(statement, 3, entry.parentID);
        } else {
            sqlite3_bind_null(statement, 3);
        }

        sqlite3_bind_int(statement, 4, entry.isDir ? 1 : 0);
        sqlite3_bind_int64(statement, 5, static_cast<int64_t>(entry.mtime));
        sqlite3_bind_text(statement, 6, entry.path.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            std::cerr << "Failed to update entry '" << entry.path << "': "
                      << sqlite3_errmsg(db) << "\n";
        }

        sqlite3_reset(statement);      // reuse the compiled statement
        sqlite3_clear_bindings(statement);
    }

    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to commit transaction: " << sqlite3_errmsg(db) << "\n";
    }

    sqlite3_finalize(statement);
    sqlite3_close(db);
}
