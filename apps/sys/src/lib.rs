use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::collections::HashMap;

extern "C" {
    fn fss_query_for(name: *const c_char) -> *mut c_char;
    fn fss_query_like(pattern: *const c_char) -> *mut c_char;
    fn fss_query_extension(ext: *const c_char) -> *mut c_char;

    #[link_name = "fetch_index_metadata"]   // links to the CPP 'fetch_index_metadata' - avoids naming collisions
    fn fetch_index_metadata_raw() -> *mut c_char;
    fn fss_free(s: *mut c_char);
}


pub fn query_for(name: &str) -> Vec<String> {
    let c_name = CString::new(name).unwrap();
    unsafe {
        let raw = fss_query_for(c_name.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        fss_free(raw);
        text.lines().map(String::from).collect()
    }
}


pub fn query_like(pattern: &str) -> Vec<String> {
    let c_pattern = CString::new(pattern).unwrap();
    unsafe {
        let raw = fss_query_like(c_pattern.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        fss_free(raw);
        text.lines().map(String::from).collect()
    }
}

pub fn query_extension(ext: &str) -> Vec<String> {
    let c_ext = CString::new(ext).unwrap();
    unsafe {
        let raw = fss_query_extension(c_ext.as_ptr());
        if raw.is_null() { return Vec::new(); }
        let text = CStr::from_ptr(raw).to_string_lossy().into_owned();
        
        fss_free(raw);

        text.lines().map(String::from).collect()
    }
}


pub fn fetch_index_metadata() -> HashMap<String, String> {
    // let c_root = CString::new(root).unwrap();
    unsafe {
        let raw = fetch_index_metadata_raw( /* c_root.as_ptr() */);
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
