#include <sqlite3.h>
#include <iostream>
#include <string>
#include "fss.hpp"
#include "exception.hpp"



bool DBExists(string DBPath) { 
    std::filesystem::path dbPath = DBPath;
    return std::filesystem::exists(dbPath); 
}


void clearDB(string root) {
    string path = DBPath(root);
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

void execSQL(sqlite3* db, const char* command) {
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, command, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errorMsg ? errorMsg : "unknown SQL error";
        sqlite3_free(errorMsg);
        throw FSSException( FSS_STATUS::SqlQueryFail, msg );
    }
}

sqlite3* openDB(string DBPath) { 
    sqlite3* db;
    int returncode = sqlite3_open(DBPath.c_str(), &db);

    if (returncode != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_close(db); // still close even though open "failed" — sqlite3_open
                            // allocates a handle even on failure and needs freeing
        throw FSSException(FSS_STATUS::DbOpen, msg);
    }
    return db;
}

// small RAII helper so every function below can't accidentally leak `db`
// on an exception path — closes on scope exit no matter how we leave
struct DbGuard {
    sqlite3* db;
    explicit DbGuard(sqlite3* d) : db(d) {}
    ~DbGuard() { if (db) sqlite3_close(db); }
    DbGuard(const DbGuard&) = delete;
    DbGuard& operator=(const DbGuard&) = delete;
};

// small RAII helper for prepared statements, same reasoning
struct StmtGuard {
    sqlite3_stmt* stmt;
    explicit StmtGuard(sqlite3_stmt* s) : stmt(s) {}
    ~StmtGuard() { if (stmt) sqlite3_finalize(stmt); }
    StmtGuard(const StmtGuard&) = delete;
    StmtGuard& operator=(const StmtGuard&) = delete;
};

void initDB(string root, string DBPath) {

    sqlite3* db = openDB(DBPath);
    DbGuard guard(db);


    const char* create_index_table = 
        "CREATE TABLE IF NOT EXISTS files ("
        "   id         INTEGER PRIMARY KEY,"
        "   path       TEXT NOT NULL UNIQUE,"
        "   filename   TEXT NOT NULL,"
        "   extension  TEXT,"
        "   parent_id  INTEGER REFERENCES files(id),"
        "   is_dir     INTEGER NOT NULL DEFAULT 0,"
        "   mtime      INTEGER NOT NULL"
        ")";

    const char* create_metadata_table = 
        "CREATE TABLE IF NOT EXISTS index_metadata ("
        "   id              INTEGER NOT NULL DEFAULT 1 CHECK (id = 1),"
        "   root            TEXT NOT NULL,"
        "   last_update     INTEGER NOT NULL,"
        "   CONSTRAINT pk_metadata PRIMARY KEY (id)"
        ")";

    const char* create_indexes =
        "CREATE INDEX IF NOT EXISTS idx_files_filename ON files(filename); "
        "CREATE INDEX IF NOT EXISTS idx_files_parent ON files(parent_id); "
        "CREATE INDEX IF NOT EXISTS idx_files_extension ON files(extension);";

        
    const char* metadata =
        "INSERT INTO index_metadata (root, last_update) "
        "VALUES (?, ?) "
        // "ON CONFLICT (id) DO UPDATE SET "
        // "   root = excluded.root, "             // excluded are the values that tried but failed to be inserted
        // "   last_update = excluded.last_update";
    ;

    execSQL(db, create_index_table);
    execSQL(db, create_metadata_table);
    execSQL(db, create_indexes);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, metadata, -1, &stmt, nullptr) != SQLITE_OK) {
        throw FSSException(FSS_STATUS::SqlQueryFail, "Could not compile SQL statement to update index metadata.");
    }

    sqlite3_bind_text(stmt, 1, root.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, epoch_now());
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw FSSException(FSS_STATUS::SqlQueryFail, "Could not initialize metadata for index.");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void insertFileEntries(const std::vector<FileEntry>& files, string DBPath) {
    const char* insert_sql =
        "INSERT INTO files (id, path, filename, extension, parent_id, is_dir, mtime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT (id) DO "
        "UPDATE SET mtime = ?;";

    
    sqlite3* db = openDB(DBPath);
    DbGuard guard(db);
    
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw FSSException(FSS_STATUS::SqlQueryFail,
            std::string("Failed to prepare insert: ") + sqlite3_errmsg(db));
    }
    StmtGuard stmtGuard(stmt);
    execSQL(db, "BEGIN TRANSACTION;");

    std::vector<std::string> failedPaths; 
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
        sqlite3_bind_int64(stmt, 8, static_cast<sqlite3_int64>(f.mtime));

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            failedPaths.push_back(f.path);
        }
        sqlite3_reset(stmt);
    }

    execSQL(db, "COMMIT;");
    
    if (!failedPaths.empty()) {
        // surface as an error rather than silently succeeding with gaps —
        // caller (update()) decides whether a partial insert is acceptable
        throw FSSException(FSS_STATUS::SqlQueryFail,
            "Failed to insert " + std::to_string(failedPaths.size()) + " entr(y/ies), "
            "first: " + failedPaths.front());
    }
}


void updateEntries(string DBPath, const std::vector<FileEntry>& entries) {
    if (entries.empty()) return;

    sqlite3* db = openDB(DBPath);
    DbGuard dbGuard(db);

    const char* query =
        "UPDATE files "
        "SET filename = ?, extension = ?, parent_id = ?, is_dir = ?, mtime = ? "
        "WHERE path = ?";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
        throw FSSException(FSS_STATUS::SqlQueryFail,
            std::string("Failed to prepare update: ") + sqlite3_errmsg(db));
            sqlite3_close(db);
    }

    StmtGuard stmtGuard(statement);
    sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    std::vector<std::string> failedPaths;
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
            failedPaths.push_back(entry.path);
        }

        sqlite3_reset(statement);      // reuse the compiled statement
        sqlite3_clear_bindings(statement);
    }

    execSQL(db, "COMMIT;");

    if (!failedPaths.empty()) {
        throw FSSException(FSS_STATUS::SqlQueryFail,
            "Failed to update " + std::to_string(failedPaths.size()) + " entr(y/ies), "
            "first: " + failedPaths.front());
    }
}



void update_metadata_table(string root) {
    
    string path = DBPath(root);
    sqlite3* db = openDB(path);

    sqlite3_stmt* stmt = nullptr;
    const char* metadata =
        "UPDATE index_metadata "
        "SET root = ?, last_update = ? "
        "WHERE id = 1";

    if (sqlite3_prepare_v2(db, metadata, -1, &stmt, nullptr) != SQLITE_OK) {
        throw FSSException(FSS_STATUS::SqlQueryFail, "Could not compile SQL statement to update index metadata.");
    }

    sqlite3_bind_text(stmt, 1, root.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, epoch_now());

    if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
        throw FSSException(FSS_STATUS::SqlQueryFail, "Could not update metadata for index.");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}