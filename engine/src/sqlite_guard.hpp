#pragma once
#include <sqlite3.h>
#include "fss/fss.hpp"


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