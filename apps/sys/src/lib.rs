use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;


extern "C" {

    fn fss_init(root: *const c_char);
    fn fss_update(root: *const c_char);

    fn fss_query_for(root: *const c_char, name: *const c_char) -> *mut c_char;
    fn fss_query_like(root: *const c_char, pattern: *const c_char) -> *mut c_char;
    fn fss_query_extension(root: *const c_char, ext: *const c_char) -> *mut c_char;

    #[link_name = "fetch_index_metadata"]   // links to the CPP 'fetch_index_metadata' - avoids naming collisions
    fn fetch_index_metadata_raw(root: *const c_char) -> *mut c_char;
    fn fss_free(s: *mut c_char);
}



pub fn init_index(root: &PathBuf) {
    match root.to_str() {
        Some(r) => {
            let c_root = CString::new( r ).unwrap();
            unsafe { fss_init(c_root.as_ptr()); }
        },
        None => eprintln!("Unable to convert root ({}) of type &PathBuf to string", root.display()),
    }

}


pub fn update_index(root: &PathBuf) {
    match root.to_str() {
        Some(r) => {
            let c_root = CString::new( r ).unwrap();
            unsafe { fss_update(c_root.as_ptr()); }
        },
        None => eprintln!("Unable to convert root ({}) of type &PathBuf to string", root.display()),
    }
}


pub fn query_for(root: &str, name: &str) -> Vec<String> {
    let c_name = CString::new(name).unwrap();
    let c_root = CString::new(root).unwrap();
    unsafe {
        let raw = fss_query_for(c_root.as_ptr(), c_name.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        fss_free(raw);
        text.lines().map(String::from).collect()
    }
}


pub fn query_like(root: &str, pattern: &str) -> Vec<String> {
    let c_pattern = CString::new(pattern).unwrap();
    let c_root = CString::new(root).unwrap();
    unsafe {
        let raw = fss_query_like(c_root.as_ptr(), c_pattern.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        fss_free(raw);
        text.lines().map(String::from).collect()
    }
}

pub fn query_extension(root: &str, ext: &str) -> Vec<String> {
    let c_ext = CString::new(ext).unwrap();
    let c_root = CString::new(root).unwrap();
    unsafe {
        let raw = fss_query_extension(c_root.as_ptr(), c_ext.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        
        fss_free(raw);

        text.lines().map(String::from).collect()
    }
}


pub fn fetch_index_metadata(root: &str) -> HashMap<String, String> {
    let c_root = CString::new(root).unwrap();
    unsafe {
        let raw = fetch_index_metadata_raw( c_root.as_ptr() );
        if raw.is_null() { 
            return HashMap::new(); 
        }

        let content = CStr::from_ptr(raw).to_string_lossy().into_owned();
        fss_free(raw);

        let map = content
            .lines()
            .filter_map(|line| line.split_once('='))
            .map(|(k, v)| (k.to_string(), v.to_string()))
            .collect();
        map
    }
}



pub fn search_for(root: &str, text: &str) -> Vec<PathBuf> {
    let mut seen: HashSet<PathBuf> = HashSet::new();  // Removes duplicate items
    let mut list_items: Vec<PathBuf> = Vec::new();

    for r in query_for(root, text) {
        let p = PathBuf::from(r);
        if seen.insert(p.clone()) {
            list_items.push(p);
        }
    }
    for r in query_like(root, text) {
        let p = PathBuf::from(r);
        if seen.insert(p.clone()) {
            list_items.push(p);
        }
    }
    for r in query_extension(root, text) {
        let p = PathBuf::from(r);
        if seen.insert(p.clone()) {
            list_items.push(p);
        }
    }

    list_items
}
