use std::io;
use std::process::Command;

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
        KeyCode::Char('q') if key.modifiers.contains(KeyModifiers::CONTROL) => {
            app.mode = Mode::Configuring;
        }
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
                    app.options_path = Some(path.clone());
                    app.options_selected = 0;
                    app.mode = Mode::Options;
                    app.banner = None;
                }
            }
        }
        KeyCode::Char(c) => {
            app.input.push(c);
            if !app.input.trim().is_empty() {
                app.submit();
                app.list_state.select(Some(0));
            }
        }
        KeyCode::Backspace => {
            app.input.pop();
            if !app.input.trim().is_empty() {
                app.submit();
                app.list_state.select(Some(0));
            }
        }

        _ => {}
    }
    Ok(AppSignal::Continue)
}

pub(crate) fn handle_options_key(
    key: crossterm::event::KeyEvent,
    app: &mut App,
) -> io::Result<()> {
    match key.code {
        KeyCode::Esc => {
            app.mode = Mode::Browsing;
            app.options_path = None;
        }
        KeyCode::Up => {
            if app.options_selected > 0 {
                app.options_selected -= 1;
            }
        }
        KeyCode::Down => {
            if app.options_selected < 2 {
                app.options_selected += 1;
            }
        }
        KeyCode::Enter => {
            if let Some(path) = app.options_path.clone() {
                match app.options_selected {
                    0 => {
                        // Copy absolute path
                        let abs = std::fs::canonicalize(&path).unwrap_or(path.clone());
                        let abs_str = abs.to_string_lossy().to_string();
                        let _ = Command::new("cmd")
                            .args(["/C", "echo", &abs_str, "|", "clip"])
                            .output();
                        app.banner = Some(Banner {
                            message: "Copied absolute path".to_string(),
                            kind: BannerType::Info,
                        });
                    }
                    1 => {
                        // Copy relative path
                        let rel_str = path.to_string_lossy().to_string();
                        let _ = Command::new("cmd")
                            .args(["/C", "echo", &rel_str, "|", "clip"])
                            .output();
                        app.banner = Some(Banner {
                            message: "Copied relative path".to_string(),
                            kind: BannerType::Info,
                        });
                    }
                    2 => {
                        // Open in default editor
                        match std::fs::read_to_string(&path) {
                            Ok(content) => {
                                app.editor_state = EditorState::new(Lines::from(content));
                                app.editing_path = Some(path.clone());
                                app.mode = Mode::Editing;
                                app.banner = None;
                            }
                            Err(_e) => {
                                app.banner = Some(Banner {
                                    message: format!(
                                        "WARNING: Can't open {}: Check file permissions and type (artifact, directory, executable, etc)",
                                        path.display(),
                                    ),
                                    kind: BannerType::Warning,
                                });
                                app.mode = Mode::Browsing;
                                app.options_path = None;
                                return Ok(());
                            }
                        }
                    }
                    _ => {}
                }
            }
            app.mode = Mode::Browsing;
            app.options_path = None;
        }
        _ => {}
    }
    Ok(())
}
