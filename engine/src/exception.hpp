#pragma once
#include <stdexcept>
#include <string>
#include <ostream>

enum class FSS_STATUS : int {
    Ok = 0,
    DbOpen = 1,
    SqlQueryFail = 2,
    CrawlErr = 3,
    InsertErr = 4,
    OtherErr = 5,
};

struct FSSException : std::runtime_error {
    FSS_STATUS status;
    FSSException(FSS_STATUS s, const std::string& msg)
        : std::runtime_error(msg), status(s) {}
};



/// @brief (fss) status to string
/// @param status fss state
/// @return name as a string
inline const char* stos(FSS_STATUS status) {
    switch (status) {
        case FSS_STATUS::Ok:            return "Ok";
        case FSS_STATUS::DbOpen:        return "DbOpen";
        case FSS_STATUS::SqlQueryFail:  return "SqlQueryFail";
        case FSS_STATUS::CrawlErr:      return "CrawlErr";
        case FSS_STATUS::InsertErr:     return "InsertErr";
        case FSS_STATUS::OtherErr:      return "OtherErr";
        default:                        return "Unknown";
    }
}


struct FSS_RESULT {
    FSS_STATUS status;
    char* message;

    bool ok() const {
        return status == FSS_STATUS::Ok;
    }

    // convenience for logging/tests
    std::string describe() const {
        return stos(status) + (message ? std::string(" — ") + message : "");
    }
};

inline std::ostream& operator<<(std::ostream& os, FSS_STATUS status) {
    return os << stos(status);
}