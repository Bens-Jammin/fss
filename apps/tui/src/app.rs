use std::path::PathBuf;

use edtui::{EditorEventHandler, EditorState};
use ratatui::widgets::ListState;

use crate::ui_utils::search_for;

pub enum Mode {
    Editing,
    Browsing,
}

pub enum AppSignal {
    Continue,
    Quit,
}

pub enum BannerType {
    Failure,
    Warning,
    Info,
}

pub struct Banner {
    pub message: String,
    pub kind: BannerType,
}

pub struct App {
    pub input: String,
    pub list_state: ListState,
    pub results: Vec<PathBuf>,
    pub mode: Mode,
    pub editor_state: EditorState,
    pub editor_handler: EditorEventHandler,
    pub editing_path: Option<PathBuf>,
    pub banner: Option<Banner>,
}

impl App {
    pub fn new() -> Self {
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

    pub fn submit(&mut self) {
        self.results = search_for(&self.input);
        // self.input.clear();
        self.banner = None;
        if self.results.is_empty() {
            self.list_state.select(None);
        } else {
            self.list_state.select(Some(0));
        }
    }
}