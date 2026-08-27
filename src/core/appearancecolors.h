#ifndef STRAWBERRY_APPEARANCECOLORS_H
#define STRAWBERRY_APPEARANCECOLORS_H

#include "constants/appearancesettings.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace AppearanceColors {

struct ColorRole {
  const char *key = nullptr;
  const char *title = nullptr;
  const char *dark_hex = nullptr;
};

inline const std::vector<ColorRole> &Roles() {
  static const std::vector<ColorRole> roles = {
      {AppearanceSettings::kColorWindow, "Window", "#353535"},
      {AppearanceSettings::kColorWindowText, "Window text", "#f0f0f0"},
      {AppearanceSettings::kColorBase, "Base", "#232323"},
      {AppearanceSettings::kColorAlternateBase, "Alternate base", "#353535"},
      {AppearanceSettings::kColorText, "Text", "#f0f0f0"},
      {AppearanceSettings::kColorButton, "Button", "#353535"},
      {AppearanceSettings::kColorButtonText, "Button text", "#f0f0f0"},
      {AppearanceSettings::kColorBrightText, "Bright text", "#ff5050"},
      {AppearanceSettings::kColorPlaceholderText, "Placeholder text", "#8c8c8c"},
      {AppearanceSettings::kColorToolTipBase, "Tooltip base", "#353535"},
      {AppearanceSettings::kColorToolTipText, "Tooltip text", "#f0f0f0"},
  };
  return roles;
}

inline const char *DarkHex(const char *key) {
  for (const auto &role : Roles()) {
    if (key && role.key && std::string(role.key) == key) {
      return role.dark_hex;
    }
  }
  return "#353535";
}

inline std::string HexOrEmpty(const std::map<std::string, std::string> &colors, const char *key) {
  const auto it = colors.find(key);
  return it == colors.end() ? std::string() : it->second;
}

inline std::string BuildPaletteCss(bool use_custom, const std::map<std::string, std::string> &colors) {
  if (!use_custom) {
    return {};
  }
  const std::string window = HexOrEmpty(colors, AppearanceSettings::kColorWindow);
  const std::string window_text = HexOrEmpty(colors, AppearanceSettings::kColorWindowText);
  const std::string base = HexOrEmpty(colors, AppearanceSettings::kColorBase);
  const std::string alt = HexOrEmpty(colors, AppearanceSettings::kColorAlternateBase);
  const std::string text = HexOrEmpty(colors, AppearanceSettings::kColorText);
  const std::string button = HexOrEmpty(colors, AppearanceSettings::kColorButton);
  const std::string button_text = HexOrEmpty(colors, AppearanceSettings::kColorButtonText);
  const std::string bright = HexOrEmpty(colors, AppearanceSettings::kColorBrightText);
  const std::string placeholder = HexOrEmpty(colors, AppearanceSettings::kColorPlaceholderText);
  const std::string tip_base = HexOrEmpty(colors, AppearanceSettings::kColorToolTipBase);
  const std::string tip_text = HexOrEmpty(colors, AppearanceSettings::kColorToolTipText);
  std::string css;
  if (!window.empty() || !window_text.empty()) {
    css += ".strawberry-main {";
    if (!window.empty()) {
      css += " background-color: " + window + ";";
    }
    if (!window_text.empty()) {
      css += " color: " + window_text + ";";
    }
    css += " }";
  }
  if (!base.empty() || !text.empty()) {
    css += ".strawberry-main list, .strawberry-main entry, .strawberry-main textview {";
    if (!base.empty()) {
      css += " background-color: " + base + ";";
    }
    if (!text.empty()) {
      css += " color: " + text + ";";
    }
    css += " }";
  }
  if (!alt.empty()) {
    css += ".strawberry-main row:nth-child(even) { background-color: " + alt + "; }";
  }
  if (!button.empty() || !button_text.empty()) {
    css += ".strawberry-main button {";
    if (!button.empty()) {
      css += " background-color: " + button + ";";
    }
    if (!button_text.empty()) {
      css += " color: " + button_text + ";";
    }
    css += " }";
  }
  if (!bright.empty()) {
    css += ".strawberry-main .error { color: " + bright + "; }";
  }
  if (!placeholder.empty()) {
    css += ".strawberry-main placeholder { color: " + placeholder + "; }";
  }
  if (!tip_base.empty() || !tip_text.empty()) {
    css += "tooltip {";
    if (!tip_base.empty()) {
      css += " background-color: " + tip_base + ";";
    }
    if (!tip_text.empty()) {
      css += " color: " + tip_text + ";";
    }
    css += " }";
  }
  return css;
}

inline std::string BuildTabBarCss(bool system_color, bool gradient, const std::string &color) {
  if (system_color || color.empty()) {
    return {};
  }
  std::string css = ".strawberry-tabbar { background-color: " + color + ";";
  if (gradient) {
    css += " background-image: linear-gradient(to bottom, alpha(white, 0.18), alpha(black, 0.12));";
  }
  css += " }";
  return css;
}

inline std::string BuildPlayingSongCss(const std::string &color) {
  if (color.empty()) {
    return {};
  }
  return ".playlist-playing, .playlist-playing label { color: " + color + "; }";
}

inline std::string BackgroundSizeCss(bool stretch, bool keep_aspect, bool do_not_cut, int max_size) {
  if (!stretch) {
    if (max_size > 0) {
      return std::to_string(max_size) + "px auto";
    }
    return "auto";
  }
  if (!keep_aspect) {
    return "100% 100%";
  }
  return do_not_cut ? "contain" : "cover";
}

struct IconSizes {
  int tabbar_small = AppearanceSettings::kDefaultIconSizeTabbarSmallMode;
  int tabbar_large = AppearanceSettings::kDefaultIconSizeTabbarLargeMode;
  int play_controls = AppearanceSettings::kDefaultIconSizePlayControlButtons;
  int playlist_buttons = AppearanceSettings::kDefaultIconSizePlaylistButtons;
  int left_panel = AppearanceSettings::kDefaultIconSizeLeftPanelButtons;
  int configure = AppearanceSettings::kDefaultIconSizeConfigureButtons;
};

inline int ClampIcon(int value, int fallback) { return value < 8 ? fallback : std::min(value, 128); }

inline IconSizes ClampIconSizes(IconSizes sizes) {
  sizes.tabbar_small = ClampIcon(sizes.tabbar_small, AppearanceSettings::kDefaultIconSizeTabbarSmallMode);
  sizes.tabbar_large = ClampIcon(sizes.tabbar_large, AppearanceSettings::kDefaultIconSizeTabbarLargeMode);
  sizes.play_controls = ClampIcon(sizes.play_controls, AppearanceSettings::kDefaultIconSizePlayControlButtons);
  sizes.playlist_buttons = ClampIcon(sizes.playlist_buttons, AppearanceSettings::kDefaultIconSizePlaylistButtons);
  sizes.left_panel = ClampIcon(sizes.left_panel, AppearanceSettings::kDefaultIconSizeLeftPanelButtons);
  sizes.configure = ClampIcon(sizes.configure, AppearanceSettings::kDefaultIconSizeConfigureButtons);
  return sizes;
}

inline std::string BuildIconSizeCss(const IconSizes &sizes) {
  const IconSizes clamped = ClampIconSizes(sizes);
  return ".strawberry-tabbar image { -gtk-icon-size: " + std::to_string(clamped.tabbar_small) +
         "px; min-width: " + std::to_string(clamped.tabbar_small) + "px; min-height: " + std::to_string(clamped.tabbar_small) +
         "px; }"
         ".strawberry-play-controls button image { -gtk-icon-size: " +
         std::to_string(clamped.play_controls) + "px; }"
         ".strawberry-playlist-buttons image { -gtk-icon-size: " +
         std::to_string(clamped.playlist_buttons) + "px; }"
         ".strawberry-left-panel image { -gtk-icon-size: " + std::to_string(clamped.left_panel) +
         "px; }"
         ".strawberry-configure-buttons image { -gtk-icon-size: " +
         std::to_string(clamped.configure) + "px; }";
}

}  // namespace AppearanceColors

#endif
