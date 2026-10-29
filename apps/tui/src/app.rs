use std::{fs, path::PathBuf};

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
    /// issues queries against it �?" never an update, never a rebuild.
    pub index: Index,
    pub root: PathBuf,
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
    pub fn new(index: Index, root: PathBuf) -> Self {
        Self {
            index,
            root,
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

    /// Returns the index root as an absolute path suitable for display.
    ///
    /// Canonicalizing resolves `.`, `..`, and symlinks, but it fails if the
    /// path no longer exists, so fall back to the root exactly as given.
    pub fn idx_abs_path(&self) -> String {
        let path = fs::canonicalize(&self.root).unwrap_or_else(|_| self.root.clone());

        // Lossy conversion avoids panicking if the path is not valid UTF-8.
        let display = path.to_string_lossy();

        // On Windows, canonical paths start with `\\?\`; hide that from users.
        display
            .strip_prefix(r"\\?\")
            .unwrap_or(&display)
            .to_string()
    }
}