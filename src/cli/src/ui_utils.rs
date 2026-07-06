use crate::fss::{query_for, query_like, query_extension};
use std::path::PathBuf;



pub(crate) fn search_for(text: &str) -> Vec<PathBuf> {
    let mut list_items: Vec<PathBuf> = Vec::new();

    // -- check exact match ---
    let exact_match_results = query_for(text);
    for r in exact_match_results {
        list_items.push( PathBuf::from( r ));
    }
    if !list_items.is_empty() { return list_items; }


    // --- check for a rough match ---
    let like_match_results = query_like(text);
    for r in like_match_results {
        list_items.push( PathBuf::from( r ));
    }
    if !list_items.is_empty() { return list_items; }


    // --- try matching extensions ---
    let ext_match_results = query_extension(text);
    for r in ext_match_results {
        list_items.push( PathBuf::from( r ));
    }
    if !list_items.is_empty() { return list_items; }


    list_items
}

