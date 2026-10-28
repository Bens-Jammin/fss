use ratatui::{
    Frame, layout::{Alignment, Constraint, Direction, Layout, Rect}, text::{Line, Span}, widgets::{Block, Borders, Clear, List, ListItem, Paragraph, Widget},
};

use edtui::{EditorView, LineNumbers};

use crate::app::{App, Mode};
use crate::theme;

pub fn ui(f: &mut Frame, app: &mut App) {
    match app.mode {
        Mode::Configuring => draw_settings(f, app),
        Mode::Editing => draw_editing(f, app),
        Mode::Browsing => draw_browsing(f, app),
        Mode::Options => {
            // Keep the search results visible behind the context menu.
            draw_browsing(f, app);
            draw_options(f, app);
        }
    }

    draw_banner(f, app);
}



fn draw_settings(f: &mut Frame, app: &mut App) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(0),    // settings content
            Constraint::Length(1), // controls
        ])
        .split(f.area());


    // Settings Content with Border
    let settings_block = Block::default()
        .title(" Configuration ") // Optional section title
        .borders(Borders::ALL)
        .style(theme::block_style())
        .border_style(theme::border_style())
        .title_style(theme::title());


    let metadata = app.index.metadata();

    let mut settings = vec![];
    for (k, v) in metadata.iter() {
        settings.push( Line::from(format!("{k}: {v}")) );
    }

    let settings_content = Paragraph::new(settings)
    .block(settings_block);

    // Fixed variable name: settings_content (was `text`)
    f.render_widget(settings_content, chunks[0]);

    // Controls
    let controls = Paragraph::new(Line::from(vec![
        Span::styled("Esc", theme::control_key()),
        Span::raw(": Quit   "),
    ]));
    f.render_widget(controls, chunks[1]);
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
        theme::filename(),
    )]))
    .alignment(Alignment::Center);
    f.render_widget(title, chunks[0]);

    EditorView::new(&mut app.editor_state)
        .theme(theme::editor_theme())
        .line_numbers(LineNumbers::Absolute)
        .render(chunks[1], f.buffer_mut());

    let controls = Paragraph::new(Line::from(vec![
        Span::styled("Esc", theme::control_key()),
        Span::raw(": Back to browsing   "),
        Span::styled("Ctrl+S", theme::control_key()),
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
        vec![ListItem::new(format!("No Results for '{}'.", &app.input)).style(theme::muted())]
    } else {
        app.results
            .iter()
            .map(|path| ListItem::new(path.display().to_string()).style(theme::text()))
            .collect()
    };

    let list = List::new(items).block(
        Block::default()
            .title("Search Results")
            .borders(Borders::ALL)
            .style(theme::block_style())
            .border_style(theme::border_style())
            .title_style(theme::title()),
    );

    if app.results.is_empty() {
        f.render_widget(list, chunks[0]);
    } else {
        let list = list
            .highlight_style(theme::selected())
            .highlight_symbol("> ");
        f.render_stateful_widget(list, chunks[0], &mut app.list_state);
    }

    // --- bottom area: the input box ---
    let input = Paragraph::new(app.input.as_str())
        .style(theme::input())
        .block(
            Block::default()
                .title("Input")
                .borders(Borders::ALL)
                .style(theme::block_style())
                .border_style(theme::border_style())
                .title_style(theme::title()),
        );
    f.render_widget(input, chunks[1]);

    let controls = Paragraph::new(Line::from(vec![
        Span::styled("Enter", theme::control_key()),
        Span::raw(": Search   "),
        Span::styled("Ctrl+Q", theme::control_key()),
        Span::raw(": Settings   "),
        Span::styled("Ctrl+X", theme::control_key()),
        Span::raw(": Quit   "),
        Span::styled("Up/Down arrow", theme::control_key()),
        Span::raw(": Move Cursor    "),
        Span::styled("Right arrow", theme::control_key()),
        Span::raw(": Select   "),
    ]));
    f.render_widget(controls, chunks[2]);

    // put the terminal cursor at the end of the typed text
    f.set_cursor_position((
        chunks[1].x + app.input.len() as u16 + 1,
        chunks[1].y + 1,
    ));
}

fn draw_options(f: &mut Frame, app: &mut App) {
    let area = f.area();
    let popup_width = if area.width > 60 { 50 } else { area.width.saturating_sub(4) };
    let popup_height = 6;
    let controls_height = 1;

    // Mirror the browsing layout so the menu can be anchored to the selected row.
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(0),
            Constraint::Length(3),
            Constraint::Length(1),
        ])
        .split(area);
    let list_area = chunks[0];

    let selected = app.list_state.selected().unwrap_or(0);
    let visible_row = selected.saturating_sub(app.list_state.offset());
    let anchor_y = list_area.y + 1 + visible_row as u16;

    // Keep the menu fully on screen, like a context menu that flips when it
    // would otherwise overflow.
    let total_height = popup_height + controls_height;
    let max_y = area.height.saturating_sub(total_height);
    let y = anchor_y.min(max_y);
    let x = area.width.saturating_sub(popup_width + 2);
    let popup_area = Rect {
        x,
        y,
        width: popup_width,
        height: popup_height,
    };

    let options = vec![
        "Copy absolute path",
        "Copy relative path",
        "Open in default editor",
    ];

    let items: Vec<ListItem> = options
        .iter()
        .enumerate()
        .map(|(i, s)| {
            if i == app.options_selected {
                ListItem::new(Line::from(vec![Span::styled(
                    format!("> {}", s),
                    theme::option_selected(),
                )]))
            } else {
                ListItem::new(format!("  {}", s)).style(theme::text())
            }
        })
        .collect();

    let block = Block::default()
        .title("Options")
        .borders(Borders::ALL)
        .style(theme::block_style())
        .border_style(theme::border_style())
        .title_style(theme::title());
    let list = List::new(items).block(block);

    f.render_widget(Clear, popup_area);
    f.render_widget(list, popup_area);

    let controls = Paragraph::new(Line::from(vec![
        Span::styled("▲/▼", theme::control_key()),
        Span::raw(": Nav  "),
        Span::styled("Enter", theme::control_key()),
        Span::raw(": Select  "),
        Span::styled("Esc", theme::control_key()),
        Span::raw(": Cancel"),
    ]));
    let controls_area = Rect {
        x,
        y: y + popup_height,
        width: popup_width,
        height: controls_height,
    };
    f.render_widget(Clear, controls_area);
    f.render_widget(controls, controls_area);
}

fn draw_banner(f: &mut Frame, app: &App) {
    if let Some(banner) = &app.banner {
        let banner_area = Rect {
            x: f.area().x,
            y: f.area().y,
            width: f.area().width,
            height: 1,
        };

        let banner_widget = Paragraph::new(banner.message.as_str())
            .style(theme::banner(&banner.kind))
            .alignment(Alignment::Left);
        f.render_widget(banner_widget, banner_area);
    }
}