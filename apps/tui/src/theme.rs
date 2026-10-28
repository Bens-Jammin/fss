use edtui::EditorTheme;
use ratatui::style::{Color, Modifier, Style};

use crate::app::BannerType;

pub const GREEN      : Color = Color::Rgb( 0x57 , 0xBB , 0x8a );
pub const BLUE       : Color = Color::Rgb( 0x46 , 0xBD , 0xC6 );
pub const YELLOW     : Color = Color::Rgb( 0xFF , 0xD6 , 0x66 );
pub const ORANGE     : Color = Color::Rgb( 0xE6 , 0x74 , 0x38 );
pub const RED_ORANGE : Color = Color::Rgb( 0xEA , 0x43 , 0x35 );
pub const RED        : Color = Color::Rgb( 0xDA , 0x23 , 0x15 );
pub const DARK_BLUE  : Color = Color::Rgb( 0x37 , 0x3F , 0x6B );

pub const FOREGROUND : Color = Color::Rgb( 0xE7 , 0xE7 , 0xE7 );
pub const BACKGROUND : Color = Color::Rgb( 0x1E , 0x1E , 0x1E );

pub fn highlight() -> Color { BLUE }

pub fn text() -> Style {
    Style::default().fg(FOREGROUND)
}

pub fn muted() -> Style {
    Style::default().fg(DARK_BLUE)
}

pub fn accent() -> Style {
    Style::default().fg(BLUE)
}

pub fn title() -> Style {
    Style::default().fg(BLUE).add_modifier(Modifier::BOLD)
}

pub fn block_style() -> Style {
    Style::default().fg(FOREGROUND)
}

pub fn border_style() -> Style {
    Style::default().fg(DARK_BLUE)
}

pub fn input() -> Style {
    Style::default().fg(YELLOW)
}

pub fn control_key() -> Style {
    Style::default().fg(YELLOW).add_modifier(Modifier::BOLD)
}

pub fn control_label() -> Style {
    Style::default().fg(FOREGROUND)
}

pub fn selected() -> Style {
    Style::default().fg(BACKGROUND).bg(BLUE).add_modifier(Modifier::BOLD)
}

pub fn filename() -> Style {
    Style::default().fg(BLUE).add_modifier(Modifier::BOLD)
}

pub fn option_selected() -> Style {
    Style::default().fg(YELLOW).add_modifier(Modifier::BOLD)
}

pub fn banner(kind: &BannerType) -> Style {
    let background = match kind {
        BannerType::Failure => RED,
        BannerType::Warning => ORANGE,
        BannerType::Info => BLUE,
    };
    Style::default().fg(BACKGROUND).bg(background).add_modifier(Modifier::BOLD)
}

pub fn editor_theme() -> EditorTheme<'static> {
    EditorTheme::default()
        .base(Style::default().fg(FOREGROUND).bg(BACKGROUND))
        .selection_style(Style::default().fg(BACKGROUND).bg(YELLOW))
        .line_numbers_style(muted())
        .cursor_style(Style::default().fg(BACKGROUND).bg(BLUE))
}
