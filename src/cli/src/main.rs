use std::io;
mod fss;
use fss::query_for;

use crossterm::{
    event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyCode, KeyEventKind, KeyModifiers},
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use ratatui::{
    backend::{Backend, CrosstermBackend},
    layout::{Constraint, Direction, Layout},
    style::{Color, Style},
    text::{Line, Span},
    widgets::{Block, Borders, List, ListItem, Paragraph},
    Frame, Terminal,
};

/// App holds the current input buffer and whatever we've computed from the
/// last submitted line.
struct App {
    input: String,
    last_submitted: Option<String>,
}

impl App {
    fn new() -> Self {
        Self {
            input: String::new(),
            last_submitted: None,
        }
    }

    fn submit(&mut self) {
        self.last_submitted = Some(self.input.clone());
        self.input.clear();
    }
}

fn main() -> io::Result<()> {
    // --- terminal setup ---
    enable_raw_mode()?;
    let mut stdout = io::stdout();
    execute!(stdout, EnterAlternateScreen, EnableMouseCapture)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    let app = App::new();
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

fn run_app<B: Backend>(terminal: &mut Terminal<B>, mut app: App) -> io::Result<()> {
    loop {
        terminal.draw(|f| ui(f, &app))?;

        if let Event::Key(key) = event::read()? {
            // On some platforms key events fire twice (press + release);
            // only act on the press.
            if key.kind != KeyEventKind::Press {
                continue;
            }
            match key.code {
                KeyCode::Esc => return Ok(()),
                KeyCode::Char('c') | KeyCode::Char('q')
                    if key.modifiers.contains(KeyModifiers::CONTROL) => { return Ok(()); }
                KeyCode::Enter => {
                    if !app.input.trim().is_empty() {
                        app.submit();
                    }
                }
                KeyCode::Char(c) => app.input.push(c),
                KeyCode::Backspace => {
                    app.input.pop();
                }
                _ => {}
            }
        }
    }
}

fn ui(f: &mut Frame, app: &App) {
    // Split the screen: everything except the last 3 rows on top,
    // a 3-row box for input at the bottom.
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(0), Constraint::Length(3)])
        .split(f.area());

    // --- top area: show values derived from the last submitted line ---
    let items: Vec<ListItem> = match &app.last_submitted {
        Some(text) => {
            let results = query_for(text);

            let mut list_items = Vec::new();

            for r in results {
                list_items.push(ListItem::new(Line::from( r )));
            }

            if list_items.len() == 0 {
                list_items = vec![ 
                    ListItem::new( Line::from( format!("No results matching '{}'", text )))
                ];
            } 

            list_items
        }
        None => vec![ListItem::new(Line::from(
            "Type something below and press Enter...",
        ))],
    };

    let list = List::new(items).block(
        Block::default()
            .title("Output")
            .borders(Borders::ALL),
    );
    f.render_widget(list, chunks[0]);

    // --- bottom area: the input box ---
    let input = Paragraph::new(app.input.as_str())
        .style(Style::default().fg(Color::Cyan))
        .block(Block::default().title("Input").borders(Borders::ALL));
    f.render_widget(input, chunks[1]);

    // put the terminal cursor at the end of the typed text
    f.set_cursor_position((
        chunks[1].x + app.input.len() as u16 + 1,
        chunks[1].y + 1,
    ));
}