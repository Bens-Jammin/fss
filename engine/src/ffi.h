// Foreign Function Interface
#pragma once
extern "C" {

    typedef struct fss_handle fss_handle;   // opaque

    // Open/create the index for `root`, building it if it does not exist.
    // Does NOT run an update — call fss_handle_update() explicitly.
    fss_handle* fss_open(const char* root) noexcept;

    // Runs one incremental update for this handle. Idempotent per-search:
    // the caller decides when staleness is acceptable.
    void fss_handle_update(fss_handle* h) noexcept;

    char* fss_handle_query_for(fss_handle* h, const char* name) noexcept;
    char* fss_handle_query_like(fss_handle* h, const char* pattern) noexcept;
    char* fss_handle_query_extension(fss_handle* h, const char* ext) noexcept;

    // Fetches the index metadata (root, last_update) as "key=value" lines.
    char* fetch_index_metadata(fss_handle* h) noexcept;

    // Closes the DB connection and frees the handle.
    void fss_close(fss_handle* h) noexcept;

    void fss_free(char* str) noexcept;
}
