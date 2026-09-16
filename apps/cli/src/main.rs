use std::path::PathBuf;
use std::fs::canonicalize;
use clap::{Parser, Subcommand};
use fss_sys::{search_for, init_index};


// To run:
// 1) make lib
// 2) make cli
// 3) run:
//   - ./apps/target/release/fss_cli.exe --help
//   - ./apps/target/release/fss_cli.exe init --root /some/path


/// fss - fast fuzzy file system search
#[derive(Parser)]
#[command(name = "fss", version, about = "Fast fuzzy file system search")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Build or initialize an index
    Init {
        /// Root directory to index (defaults to the current directory)
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf,

        /// Rebuild the index even if one already exists
        #[arg(short, long)]
        force: bool,
    },
    /// Show current index state / metadata
    State {
        /// Root directory whose index metadata should be shown
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf,
    },
    /// Query the index for matching cases
    Find {
        /// Root directory to index (defaults to the current directory)
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf,

        /// Pattern to search for in the index tree. Can be a file/dir name or file extension
        #[arg(short, long, value_name = "PATTERN")]
        pattern: String,

        /// Return the file paths as absolute paths
        #[arg(short, long, action = clap::ArgAction::SetTrue)]
        absolute: bool,
    },
    /// Refresh the index
    Update {
        /// Root directory to index (defaults to the current directory)
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf
    },
    /// Sets a default index for which to perform all commands on
    Connect {
        /// Root directory to index (defaults to the current directory)
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf
    },
    /// Disconnects from an index if currently connected
    Disconnect {},
}

fn main() {
    let cli = Cli::parse();

    match cli.command {
        // TODO: wire up to fss_sys once root-parameterized FFI lands
        Commands::Init { root, force } => {
            println!("init: root={:?}, force={}", root, force);
            init_index(&root);
        }
        // TODO: call fss_sys::fetch_index_metadata() and print it
        Commands::State { root } => {
            println!("{}", root.canonicalize().unwrap().display());
            let displayable_abs_root: String = display_path(&root.canonicalize().unwrap());
            println!("state: root={:?}", displayable_abs_root);
        }
        Commands::Find { root, pattern, absolute } => {
            if pattern.trim().is_empty() { println!("Cannot pattern match on an empty pattern."); }
            // TODO: what if this root isnt indexed?

            let results = search_for(root.to_str().unwrap(), &pattern.to_string());
            if results.is_empty() {
                println!("0 Results found.");
            }
            println!("Found {} result(s):", results.len());
            if absolute {
                for r in results {
                    println!("- {}", display_path(&r.canonicalize().unwrap()));
                }
            } else {
                for r in results {
                    println!("- {}", r.display());
                }
            }
        }
        Commands::Update  { root } => { fss_update(root); },
        Commands::Connect { root } => { panic!("Not yet implemented!"); },
        Commands::Disconnect {}    => { panic!("Not yet implemented!"); },

    }
}



fn display_path(p: &std::path::Path) -> String {
    let s = p.to_string_lossy();
    s.strip_prefix(r"\\?\").unwrap_or(&s).to_string()
}