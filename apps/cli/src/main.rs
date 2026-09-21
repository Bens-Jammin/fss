use std::path::PathBuf;
use clap::{Parser, Subcommand};
use fss_sys::{search_for, init_index, update_index};
use std::time::Instant;

// To run:
// 1) make lib
// 2) make cli
// 3) run:
//   - ./apps/target/release/fss_cli.exe --help
//   - ./apps/target/release/fss_cli.exe init --root /some/path


/// fss - fast fuzzy file system search
#[derive(Parser)]
#[command(name = "fss", version, about = "Fast System Search engine")]
struct Cli {
    /// Root directory of the index (overrides the connected index)
    #[arg(short, long, value_name = "PATH", env = "FSS_ROOT", global = true)]
    root: Option<PathBuf>,

    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Init { #[arg(short, long)] force: bool },
    State,
    Find {
        #[arg(short, long, value_name = "PATTERN")]
        pattern: String,
        #[arg(short, long)]
        absolute: bool
    },
    Update,
    Connect,
    Disconnect
}

fn state_file() -> Option<PathBuf> {
    // `dirs` crate; on Windows this lands in %APPDATA%
    dirs::config_dir()
        .map(|d| 
            d.join("fss")
            .join("connected_root")
        )
}

fn resolve_root(explicit: Option<PathBuf>) -> PathBuf {
    explicit
        .or_else(connected_root)
        .unwrap_or_else(|| PathBuf::from("."))
}


fn connect(root: &PathBuf) -> std::io::Result<()> {
    let root = root.canonicalize()?;
    let file = state_file().expect("no config dir available");
    std::fs::create_dir_all(file.parent().unwrap())?;
    std::fs::write(file, root.to_string_lossy().as_bytes())
}

fn disconnect() -> std::io::Result<()> {
    let Some(file) = state_file() else { return Ok(()) };
    match std::fs::remove_file(file) {
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(()),
        other => other,
    }
}

fn connected_root() -> Option<PathBuf> {
    let file = state_file()?;
    let s = std::fs::read_to_string(file).ok()?;
    let s = s.trim();
    if s.is_empty() { return None; }
    Some(PathBuf::from(s))
}


fn main() {
    let cli = Cli::parse();
    let root: PathBuf = resolve_root(cli.root.clone());

    match cli.command {
        Commands::Init { force } => {
            init_index(&root);
        }
        Commands::State => {
            println!("{}", root.canonicalize().unwrap().display());
            let displayable_abs_root: String = display_path(&root.canonicalize().unwrap());
            println!("state: root={:?}", displayable_abs_root);
        }
        Commands::Find { pattern, absolute } => {
            if pattern.trim().is_empty() { println!("Cannot pattern match on an empty pattern."); }
            // TODO: what if this root isnt indexed?
            
            let start = Instant::now();
            let results = search_for(root.to_str().unwrap(), &pattern.to_string());
            let duration = start.elapsed();
            let result_runtime_s: f64 = (duration.as_millis() as f64) / 1000.0;

            if results.is_empty() {
                println!("0 Results found in {}s.", result_runtime_s);
            }
            if absolute {
                for r in &results {
                    println!("- {}", display_path(&r.canonicalize().unwrap()));
                }
            } else {
                for r in &results {
                    println!("- {}", display_path( &r ));
                }
            }
            println!("\nFound {} result(s) in {}s:", results.len(), result_runtime_s);
        }
        Commands::Update     {} => { update_index(&root); },
        Commands::Connect    {} => {
            if let Err(e) = connect(&root) {
                eprintln!("connect failed: {e}");
                std::process::exit(1);
            } else {
                println!("connected to {}.", display_path(&root) );
            }
        },
        Commands::Disconnect {} => {
            if let Err(e) = disconnect() {
                eprintln!("disconnect failed: {e}");
                std::process::exit(1);
            } else {
                println!("disconnected from {}.", display_path(&root) );
            }
        },

    }
}



fn display_path(p: &std::path::Path) -> String {
    let s = p.to_string_lossy();
    s.strip_prefix(r"\\?\").unwrap_or(&s).to_string()
}