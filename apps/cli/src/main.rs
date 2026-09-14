use clap::{Parser, Subcommand};

// To run:
// 1) make lib
// 2) make cli
// 3) run:
//   - ./apps/target/release/fss_cli.exe --help
//   - ./apps/target/release/fss_cli.exe init


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
    Init,
    /// Show current index state / metadata
    State,
    /// Check the index for consistency issues
    Check,
}

fn main() {
    let cli = Cli::parse();

    match cli.command {
        // TODO: wire up to fss_sys once root-parameterized FFI lands
        Commands::Init => {}
        // TODO: call fss_sys::fetch_index_metadata() and print it
        Commands::State => {}
        // TODO: validate index / DB consistency
        Commands::Check => {}
    }
}