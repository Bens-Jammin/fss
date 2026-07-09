use std::io;
use std::path::PathBuf;

mod fss;
mod ui_utils;

use ui_utils::{search_for};

use edtui::{EditorState, EditorEventHandler, EditorTheme, EditorStatusLine, EditorView, Lines, LineNumbers};
use crossterm::{
    event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyCode, KeyEventKind, KeyModifiers},
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use ratatui::{
    backend::{Backend, CrosstermBackend},
    layout::{Alignment, Constraint, Direction, Layout, Rect},
    style::{Color, Style},
    text::{Line, Span},
    widgets::{Widget, Block, Borders, List, ListItem, Paragraph, ListState},
    Frame, Terminal,
};


enum Mode {
    Editing,
    Browsing
}

enum AppSignal {
    Continue,
    Quit,
}

enum BannerType {
    Failure,
    Warning,
    Info
}

struct Banner {
    pub message: String,
    pub kind: BannerType, 
}

pub(crate) fn handle_editing_key(
    key: crossterm::event::KeyEvent,
    app: &mut App,
) -> io::Result<()> {
    if key.code == KeyCode::Char('s') && key.modifiers.contains(KeyModifiers::CONTROL) {
        if let Some(path) = &app.editing_path {
            let text: String = app.editor_state.lines.clone().into();
            std::fs::write(path, text)?;
        }
        app.mode = Mode::Browsing;
    } else if key.code == KeyCode::Esc {
        app.mode = Mode::Browsing;
    } else {
        app.editor_handler.on_key_event(key, &mut app.editor_state);
    }
    Ok(())
}

pub(crate) fn handle_browsing_key(
    key: crossterm::event::KeyEvent,
    app: &mut App,
) -> io::Result<AppSignal> {
    match key.code {
        KeyCode::Char('x') if key.modifiers.contains(KeyModifiers::CONTROL) => {
            return Ok(AppSignal::Quit);
        }
        KeyCode::Down => {
            let current = app.list_state.selected().unwrap_or(0);
            if current + 1 < app.results.len() {
                app.list_state.select(Some(current + 1));
            }
        }
        KeyCode::Up => {
            let current = app.list_state.selected().unwrap_or(0);
            if current > 0 {
                app.list_state.select(Some(current - 1));
            }
        }
        KeyCode::Right => {
            if let Some(i) = app.list_state.selected() {
                if let Some(path) = app.results.get(i) {
                    match std::fs::read_to_string(path) {
                        Ok(content) => {
                            app.editor_state = EditorState::new(Lines::from(content));
                            app.editing_path = Some(path.clone());
                            app.mode = Mode::Editing;
                            app.banner = None; // clear any stale banner
                        }
                        Err(e) => {
                            app.banner = Some(Banner {
                                message: format!("WARNING: Can't open {}: Check file permissions and type (artifact, directory, executable, etc)", path.display(), ), // use 'e' if want to show err
                                kind: BannerType::Warning,
                            });
                        }
                    }                
                }
            }
        }
        KeyCode::Enter => {
            if !app.input.trim().is_empty() {
                app.submit();
                app.list_state.select(Some(0));
            }
        }
        KeyCode::Char(c) => app.input.push(c),
        KeyCode::Backspace => {
            app.input.pop();
        }
        _ => {}
    }
    Ok(AppSignal::Continue)
}

struct App {
    input: String,
    list_state: ListState,
    results: Vec<PathBuf>,
    mode: Mode,
    editor_state: EditorState,
    editor_handler: EditorEventHandler,
    editing_path: Option<PathBuf>,
    banner: Option<Banner>,
}

impl App {
    fn new() -> Self {

        Self {
            input: String::new(),
            results: Vec::new(),
            list_state: ListState::default(),
            mode: Mode::Browsing,
            editor_state: EditorState::default(),
            editor_handler: EditorEventHandler::default(), // vim mode
            editing_path: None,
            banner: None,
        }
    }

    fn submit(&mut self) {
        self.results = search_for(&self.input);
        self.input.clear();
        self.banner = None;
        if self.results.is_empty() {
            self.list_state.select(None);
        } else {
            self.list_state.select(Some(0));
        }
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


fn run_app<B: Backend>(terminal: &mut Terminal<B>, mut app: App)
-> io::Result<()> where std::io::Error: From<<B as Backend>::Error> {
    loop {
        terminal.draw(|f| ui(f, &mut app))?;

        if let Event::Key(key) = event::read()? {
            if key.kind != KeyEventKind::Press {
                continue;
            }

            match app.mode {
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


fn ui(f: &mut Frame, app: &mut App) {

    match app.mode {
        Mode::Editing => {
            // TODO!
            // 1) enable setting customization (?)
            // 2) be allowed to quit the editor
            // 3) does it even save (?)
            // 4) show the file name at the top
            let chunks = Layout::default()
                .direction(Direction::Vertical)
                .constraints([
                    Constraint::Length(1), // filename
                    Constraint::Min(0),    // editor
                    Constraint::Length(1), // controls
                ])
                .split(f.area());
            
            
            let filename = app
                .editing_path
                .as_ref()
                .map(|p| p.display().to_string())
                .unwrap_or_else(|| "Unknown file path".to_string());

            let title = Paragraph::new(Line::from(vec![
                Span::styled(filename, Style::default().fg(Color::White))
            ]))
            .alignment(Alignment::Center);
            f.render_widget(title, chunks[0]);
            
            EditorView::new(&mut app.editor_state)
                .theme(
                    EditorTheme::default()
                        .base(Style::default().bg(Color::Black).fg(Color::White))
                        .selection_style(Style::default().bg(Color::Yellow).fg(Color::Black))
                )
                .line_numbers(LineNumbers::Absolute)
                .render(chunks[1], f.buffer_mut());

            let controls = Paragraph::new(Line::from(vec![
                Span::styled("Esc", Style::default().fg(Color::Yellow)),
                Span::raw(": Back to browsing   "),
                Span::styled("Ctrl+S", Style::default().fg(Color::Yellow)),
                Span::raw(": Save   "),
            ]));
            f.render_widget(controls, chunks[2]);

        }
        Mode::Browsing => {
            // Split the screen: everything except the last 3 rows on top,
            // a 3-row box for input at the bottom.
            let chunks = Layout::default()
                .direction(Direction::Vertical)
                .constraints([
                    Constraint::Min(0), 
                    Constraint::Length(3),  // input
                    Constraint::Length(1)
                ])
                .split(f.area());

            // --- top area: show values derived from the last submitted line ---
            let items: Vec<ListItem> = if app.results.is_empty() {
                vec![ListItem::new(
                    "No Results",
                )]
            } else {
                app.results
                    .iter()
                    .map(|path| {
                        ListItem::new(path.display().to_string())
                    })
                    .collect()
            };

            let list = List::new(items).block(
                Block::default()
                    .title("Search Results")
                    .borders(Borders::ALL),
            );
            
            if app.results.is_empty() {
                f.render_widget(list, chunks[0]);
            } else {
                let list = list.highlight_style(
                    Style::default()
                        .bg(Color::Blue)
                        .fg(Color::DarkGray)
                ).highlight_symbol("> ");
                f.render_stateful_widget(list, chunks[0], &mut app.list_state);
            }

            // --- bottom area: the input box ---
            let input = Paragraph::new(app.input.as_str())
                .style(Style::default().fg(Color::Cyan))
                .block(Block::default().title("Input").borders(Borders::ALL));
            f.render_widget(input, chunks[1]);

            let controls = Paragraph::new(Line::from(vec![
                Span::styled("Enter", Style::default().fg(Color::Yellow)),
                Span::raw(": Search   "),
                Span::styled("Ctrl+X", Style::default().fg(Color::Yellow)),
                Span::raw(": Quit   "),
                Span::styled("Up/Down arrow", Style::default().fg(Color::Yellow)),
                Span::raw(": Select   "),
                Span::styled("Right arrow", Style::default().fg(Color::Yellow)),
                Span::raw(": Open   "),
                
            ]));
            f.render_widget(controls, chunks[2]);

            // put the terminal cursor at the end of the typed text
            f.set_cursor_position((
                chunks[1].x + app.input.len() as u16 + 1,
                chunks[1].y + 1,
            ));
        }
    }

    if let Some(banner) = &app.banner {
        let color = match banner.kind {
            BannerType::Failure => Color::Red,
            BannerType::Warning => Color::Yellow,
            BannerType::Info => Color::Blue,
        };

        let banner_area = Rect {
            x: f.area().x,
            y: f.area().y,
            width: f.area().width,
            height: 1,
        };

        let banner_widget = Paragraph::new(banner.message.as_str())
            .style(Style::default().fg(Color::Black).bg(color))
            .alignment(Alignment::Left);
        f.render_widget(banner_widget, banner_area);
    }

}