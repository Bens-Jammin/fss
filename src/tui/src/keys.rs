use std::io;

use crossterm::event::{KeyCode, KeyModifiers};
use edtui::{EditorState, Lines};

use crate::app::{App, AppSignal, Banner, BannerType, Mode};

pub(crate) fn handle_configuring_key(
    key: crossterm::event::KeyEvent,
    app: &mut App,
) -> io::Result<()> {
    if key.code == KeyCode::Esc {
        app.mode = Mode::Browsing;
    }
    Ok(())
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
        KeyCode::Char('q') if key.modifiers.contains(KeyModifiers::CONTROL) => {
            app.mode = Mode::Configuring;
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
                        Err(_e) => {
                            app.banner = Some(Banner {
                                message: format!(
                                    "WARNING: Can't open {}: Check file permissions and type (artifact, directory, executable, etc)",
                                    path.display(),
                                ), // use 'e' if want to show err
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