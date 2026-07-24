use ratatui::{
    layout::{Alignment, Constraint, Direction, Layout, Rect},
    style::{Color, Style},
    text::{Line, Span},
    widgets::{Block, Borders, List, ListItem, Paragraph, Widget},
    Frame,
};

use edtui::{EditorTheme, EditorView, LineNumbers};

use crate::app::{App, BannerType, Mode};

pub fn ui(f: &mut Frame, app: &mut App) {
    match app.mode {
        Mode::Editing => draw_editing(f, app),
        Mode::Browsing => draw_browsing(f, app),
    }

    draw_banner(f, app);
}

fn draw_editing(f: &mut Frame, app: &mut App) {
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

    let title = Paragraph::new(Line::from(vec![Span::styled(
        filename,
        Style::default().fg(Color::White),
    )]))
    .alignment(Alignment::Center);
    f.render_widget(title, chunks[0]);

    EditorView::new(&mut app.editor_state)
        .theme(
            EditorTheme::default()
                .base(Style::default().bg(Color::Black).fg(Color::White))
                .selection_style(Style::default().bg(Color::Yellow).fg(Color::Black)),
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

fn draw_browsing(f: &mut Frame, app: &mut App) {
    // Split the screen: everything except the last 3 rows on top,
    // a 3-row box for input at the bottom.
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(0),
            Constraint::Length(3), // input
            Constraint::Length(1),
        ])
        .split(f.area());

    // --- top area: show values derived from the last submitted line ---
    let items: Vec<ListItem> = if app.results.is_empty() {
        vec![ListItem::new(format!("No Results for '{}'.", &app.input))]
    } else {
        app.results
            .iter()
            .map(|path| ListItem::new(path.display().to_string()))
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
        let list = list
            .highlight_style(Style::default().bg(Color::Blue).fg(Color::DarkGray))
            .highlight_symbol("> ");
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

fn draw_banner(f: &mut Frame, app: &App) {
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