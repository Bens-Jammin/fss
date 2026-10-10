use std::io;
use std::path::PathBuf;

mod app;
mod keys;
mod ui;

use fss_sys::Index;

use app::{App, AppSignal, Mode};
use keys::{handle_browsing_key, handle_editing_key, handle_configuring_key};
use ui::ui;

use crossterm::{
    event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyEventKind},
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use ratatui::{
    backend::{Backend, CrosstermBackend},
    Terminal,
};

/// Root of the index is taken from `FSS_ROOT`, falling back to the current
/// directory. The index is opened once here and reused for every keystroke.
fn resolve_root() -> PathBuf {
    std::env::var_os("FSS_ROOT")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("."))
}

fn main() -> io::Result<()> {
    let root = resolve_root();
    let index = match Index::open(&root) {
        Ok(index) => index,
        Err(e) => {
            eprintln!("cannot open index for {}: {e}", root.display());
            std::process::exit(1);
        }
    };

    // --- terminal setup ---
    enable_raw_mode()?;
    let mut stdout = io::stdout();
    execute!(stdout, EnterAlternateScreen, EnableMouseCapture)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    let app = App::new(index);
    let result = run_app(&mut terminal, app);

    // --- terminal teardown (always run, even if run_app errored) ---
    disable_raw_mode()?;
    execute!(
        terminal.backend_mut(),
        LeaveAlternateScreen,
        DisableMouseCapture
    )?;
    terminal.show_cursor()?;

    result
}

fn run_app<B: Backend>(terminal: &mut Terminal<B>, mut app: App) -> io::Result<()>
where
    std::io::Error: From<<B as Backend>::Error>,
{
    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        if let Event::Key(key) = event::read()? {
            if key.kind != KeyEventKind::Press {
                continue;
            }

            match app.mode {
                Mode::Configuring => handle_configuring_key(key, &mut app)?,
                Mode::Editing => handle_editing_key(key, &mut app)?,
                Mode::Browsing => {
                    if let AppSignal::Quit = handle_browsing_key(key, &mut app)? {
                        return Ok(());
                    }
                }
            }
        }
    }
}