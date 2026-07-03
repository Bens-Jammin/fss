// build.rs
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    // src/cli -> up two levels -> repo root, where libfssbackend.a lives
    let backend_dir = manifest_dir.join("../../build").canonicalize().unwrap();

    println!("cargo:rustc-link-search=native={}", backend_dir.display());
    println!("cargo:rustc-link-lib=static=fssbackend");
    println!("cargo:rustc-link-lib=stdc++");
    println!("cargo:rustc-link-lib=sqlite3");
}