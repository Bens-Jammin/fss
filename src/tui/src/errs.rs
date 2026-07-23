#[derive(Debug)]
pub enum FssError {
    DbOpen(String),
    SqlExec(String),
    Crawl(String),
    Insert(String),
    Unexpected(String),
}

impl std::fmt::Display for FssError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            FssError::DbOpen(m) => write!(f, "Database open failed: {m}"),
            FssError::SqlExec(m) => write!(f, "Query failed: {m}"),
            FssError::Crawl(m) => write!(f, "Filesystem scan failed: {m}"),
            FssError::Insert(m) => write!(f, "Index update failed: {m}"),
            FssError::Unexpected(m) => write!(f, "Unexpected error: {m}"),
        }
    }
}
// TODO! implement the rest of the error messages