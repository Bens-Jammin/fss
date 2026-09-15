use std::path::PathBuf;

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
    /// Check the index for consistency issues
    Check {
        /// Root directory to check
        #[arg(short, long, value_name = "PATH", default_value = ".")]
        root: PathBuf,

        /// Print detailed per-entry results
        #[arg(short, long)]
        verbose: bool,

        /// Attempt to repair issues found during the check
        #[arg(long)]
        fix: bool,
    },
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
            println!("state: root={:?}", root);
        }
        // TODO: validate index / DB consistency
        Commands::Check { root, verbose, fix } => {
            println!("check: root={:?}, verbose={}, fix={}", root, verbose, fix);
        }
    }
}