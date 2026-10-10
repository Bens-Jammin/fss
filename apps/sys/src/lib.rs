use std::collections::{HashMap, HashSet};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::path::{Path, PathBuf};

/// Opaque handle to the C++ indexer. Never constructed on the Rust side; only
/// ever held behind a pointer returned by `fss_open`.
#[allow(non_camel_case_types)]
#[repr(C)]
pub struct fss_handle {
    _private: [u8; 0],
}

extern "C" {
    fn fss_open(root: *const c_char) -> *mut fss_handle;
    fn fss_handle_update(h: *mut fss_handle);

    fn fss_handle_query_for(h: *mut fss_handle, name: *const c_char) -> *mut c_char;
    fn fss_handle_query_like(h: *mut fss_handle, pattern: *const c_char) -> *mut c_char;
    fn fss_handle_query_extension(h: *mut fss_handle, ext: *const c_char) -> *mut c_char;

    #[link_name = "fetch_index_metadata"] // links to the CPP 'fetch_index_metadata' - avoids naming collisions
    fn fetch_index_metadata_raw(h: *mut fss_handle) -> *mut c_char;

    fn fss_close(h: *mut fss_handle);
    fn fss_free(s: *mut c_char);
}

#[derive(Debug)]
pub enum FssError {
    InvalidRoot(String),
    Open(String),
}

impl std::fmt::Display for FssError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            FssError::InvalidRoot(m) => write!(f, "Invalid root path: {m}"),
            FssError::Open(m) => write!(f, "Could not open index: {m}"),
        }
    }
}

impl std::error::Error for FssError {}

/// A long-lived handle to an open index. Open it once and reuse it for every
/// query: constructing this performs no crawl and issuing queries performs no
/// writes. Call [`Index::update`] explicitly when you want to pick up changes
/// on disk.
pub struct Index {
    handle: *mut fss_handle,
}

impl Index {
    /// Open/create the index for `root`, building it if it does not exist.
    /// Does NOT run an update — call [`Index::update`] explicitly.
    pub fn open(root: &Path) -> Result<Self, FssError> {
        let root_str = root
            .to_str()
            .ok_or_else(|| FssError::InvalidRoot(root.display().to_string()))?;

        let c_root = CString::new(root_str)
            .map_err(|_| FssError::InvalidRoot(root.display().to_string()))?;

        let handle = unsafe { fss_open(c_root.as_ptr()) };
        if handle.is_null() {
            return Err(FssError::Open(root.display().to_string()));
        }

        Ok(Self { handle })
    }

    /// Runs one incremental update for this handle. The caller decides when
    /// staleness is acceptable; queries never trigger an update themselves.
    pub fn update(&mut self) -> Result<(), FssError> {
        unsafe { fss_handle_update(self.handle) };
        Ok(())
    }

    /// Runs the exact / substring / extension queries against this handle and
    /// returns the de-duplicated union. Takes `&self`: searching never writes.
    pub fn search_for(&self, text: &str) -> Vec<PathBuf> {
        let mut seen: HashSet<PathBuf> = HashSet::new(); // Removes duplicate items
        let mut list_items: Vec<PathBuf> = Vec::new();

        for r in query_exact(self.handle, text) {
            let p = PathBuf::from(r);
            if seen.insert(p.clone()) {
                list_items.push(p);
            }
        }
        for r in query_substr(self.handle, text) {
            let p = PathBuf::from(r);
            if seen.insert(p.clone()) {
                list_items.push(p);
            }
        }
        for r in query_extension(self.handle, text) {
            let p = PathBuf::from(r);
            if seen.insert(p.clone()) {
                list_items.push(p);
            }
        }

        list_items
    }

    /// Fetches the index metadata (root, last_update) as a key/value map.
    pub fn metadata(&self) -> HashMap<String, String> {
        let raw = unsafe { fetch_index_metadata_raw(self.handle) };
        if raw.is_null() {
            return HashMap::new();
        }

        let content = unsafe { c_string(raw) };

        content
            .lines()
            .filter_map(|line| line.split_once('='))
            .map(|(k, v)| (k.to_string(), v.to_string()))
            .collect()
    }
}

impl Drop for Index {
    fn drop(&mut self) {
        unsafe { fss_close(self.handle) }
    }
}

unsafe fn collect_lines(raw: *mut c_char) -> Vec<String> {
    if raw.is_null() {
        return Vec::new();
    }
    c_string(raw).lines().map(String::from).collect()
}

unsafe fn c_string(raw: *mut c_char) -> String {
    let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
    fss_free(raw);
    text
}

fn query_exact(handle: *mut fss_handle, name: &str) -> Vec<String> {
    let Ok(c_name) = CString::new(name) else {
        return Vec::new();
    };
    unsafe { collect_lines(fss_handle_query_for(handle, c_name.as_ptr())) }
}

fn query_substr(handle: *mut fss_handle, pattern: &str) -> Vec<String> {
    let Ok(c_pattern) = CString::new(pattern) else {
        return Vec::new();
    };
    unsafe { collect_lines(fss_handle_query_like(handle, c_pattern.as_ptr())) }
}

fn query_extension(handle: *mut fss_handle, ext: &str) -> Vec<String> {
    let Ok(c_ext) = CString::new(ext) else {
        return Vec::new();
    };
    unsafe { collect_lines(fss_handle_query_extension(handle, c_ext.as_ptr())) }
}
