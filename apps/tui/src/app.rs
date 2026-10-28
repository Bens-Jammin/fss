use std::path::PathBuf;

use edtui::{EditorEventHandler, EditorState};
use ratatui::widgets::ListState;

use fss_sys::Index;

pub enum Mode {
    Editing,
    Browsing,
    Configuring,
    Options,
}

#[derive(Clone, Debug)]
pub enum OptionAction {
    CopyAbsolutePath,
    CopyRelativePath,
    OpenInDefaultEditor,
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
    /// The index is opened once for the lifetime of the app. Typing only ever
    /// issues queries against it — never an update, never a rebuild.
    pub index: Index,
    pub input: String,
    pub list_state: ListState,
    pub results: Vec<PathBuf>,
    pub mode: Mode,
    pub editor_state: EditorState,
    pub editor_handler: EditorEventHandler,
    pub editing_path: Option<PathBuf>,
    pub banner: Option<Banner>,
    pub options_path: Option<PathBuf>,
    pub options_selected: usize,
}

impl App {
    pub fn new(index: Index) -> Self {
        Self {
            index,
            input: String::new(),
            results: Vec::new(),
            list_state: ListState::default(),
            mode: Mode::Browsing,
            editor_state: EditorState::default(),
            editor_handler: EditorEventHandler::default(), // vim mode
            editing_path: None,
            banner: None,
            options_path: None,
            options_selected: 0,
        }

    }

    pub fn submit(&mut self) {
        self.results = self.index.search_for(&self.input);
        // self.input.clear();
        self.banner = None;
        if self.results.is_empty() {
            self.list_state.select(None);
        } else {
            self.list_state.select(Some(0));
        }
    }
}