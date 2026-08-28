#include "core/appearance.h"

#include "constants/appearancesettings.h"
#include "core/appearancestyle.h"
#include "core/settings.h"
#include "utilities/styleutils.h"

#include <adwaita.h>

#include <algorithm>

void Appearance::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(AppearanceSettings::kSettingsGroup);
  dark_mode_ = settings.BoolValue(AppearanceSettings::kDarkMode, AppearanceSettings::kDefaultDarkMode);
  system_icons_ = settings.BoolValue(AppearanceSettings::kSystemThemeIcons, AppearanceSettings::kDefaultSystemIcons);
  style_ = settings.Value(AppearanceSettings::kStyle);
  use_custom_colors_ = settings.BoolValue(AppearanceSettings::kUseCustomColorSet, AppearanceSettings::kDefaultUseCustomColorSet);
  custom_colors_.clear();
  for (const auto &role : AppearanceColors::Roles()) {
    custom_colors_[role.key] = settings.Value(role.key);
  }
  tabbar_system_color_ = settings.BoolValue(AppearanceSettings::kTabBarSystemColor, AppearanceSettings::kDefaultTabBarSystemColor);
  tabbar_gradient_ = settings.BoolValue(AppearanceSettings::kTabBarGradient, AppearanceSettings::kDefaultTabBarGradient);
  tabbar_color_ = settings.Value(AppearanceSettings::kTabBarColor);
  playing_song_color_ = settings.Value(AppearanceSettings::kPlaylistPlayingSongColor);
  background_filename_ = settings.Value(AppearanceSettings::kBackgroundImageFilename);
  background_type_ = settings.IntValue(AppearanceSettings::kBackgroundImageType, static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType));
  background_position_ = settings.IntValue(AppearanceSettings::kBackgroundImagePosition,
                                           static_cast<int>(AppearanceSettings::kDefaultBackgroundImagePosition));
  blur_radius_ = settings.IntValue(AppearanceSettings::kBackgroundImageBlurRadius, AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  opacity_ = settings.IntValue(AppearanceSettings::kBackgroundImageOpacityLevel, AppearanceSettings::kDefaultBackgroundImageOpacityLevel);
  background_stretch_ = settings.BoolValue(AppearanceSettings::kBackgroundImageStretch, AppearanceSettings::kDefaultBackgroundImageStretch);
  background_keep_aspect_ = settings.BoolValue(AppearanceSettings::kBackgroundImageKeepAspectRatio,
                                               AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio);
  background_do_not_cut_ = settings.BoolValue(AppearanceSettings::kBackgroundImageDoNotCut, AppearanceSettings::kDefaultBackgroundImageDoNotCut);
  background_max_size_ = settings.IntValue(AppearanceSettings::kBackgroundImageMaxSize, AppearanceSettings::kDefaultBackgroundImageMaxSize);
  icon_sizes_.tabbar_small = settings.IntValue(AppearanceSettings::kIconSizeTabbarSmallMode, AppearanceSettings::kDefaultIconSizeTabbarSmallMode);
  icon_sizes_.tabbar_large = settings.IntValue(AppearanceSettings::kIconSizeTabbarLargeMode, AppearanceSettings::kDefaultIconSizeTabbarLargeMode);
  icon_sizes_.play_controls =
      settings.IntValue(AppearanceSettings::kIconSizePlayControlButtons, AppearanceSettings::kDefaultIconSizePlayControlButtons);
  icon_sizes_.playlist_buttons =
      settings.IntValue(AppearanceSettings::kIconSizePlaylistButtons, AppearanceSettings::kDefaultIconSizePlaylistButtons);
  icon_sizes_.left_panel = settings.IntValue(AppearanceSettings::kIconSizeLeftPanelButtons, AppearanceSettings::kDefaultIconSizeLeftPanelButtons);
  icon_sizes_.configure = settings.IntValue(AppearanceSettings::kIconSizeConfigureButtons, AppearanceSettings::kDefaultIconSizeConfigureButtons);
}

void Appearance::Apply() {
  ReloadSettings();
  AdwStyleManager *manager = adw_style_manager_get_default();
  if (manager) {
    const bool dark = dark_mode_ || AppearanceStyle::ForcesDark(style_);
    adw_style_manager_set_color_scheme(manager, dark ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
  }
  const std::string style_css = AppearanceStyle::CssFor(style_);
  if (!style_css.empty()) {
    StyleUtils::LoadCss(style_css, StyleUtils::Slot::kAppearanceStyle);
  } else if (!style_.empty() && style_.find('{') != std::string::npos) {
    StyleUtils::LoadCss(style_, StyleUtils::Slot::kAppearanceStyle);
  }
  const std::string theme = ThemeCss();
  if (!theme.empty()) {
    StyleUtils::LoadCss(theme, StyleUtils::Slot::kAppearanceTheme);
  }
}

std::string Appearance::ThemeCss() const {
  return AppearanceColors::BuildPaletteCss(use_custom_colors_, custom_colors_) +
         AppearanceColors::BuildTabBarCss(tabbar_system_color_, tabbar_gradient_, tabbar_color_) +
         AppearanceColors::BuildPlayingSongCss(playing_song_color_) + AppearanceColors::BuildIconSizeCss(icon_sizes_);
}

std::string Appearance::CssUrl(const std::string &path) {
  if (path.empty()) {
    return {};
  }
  std::string url = path;
  if (url.find("://") == std::string::npos) {
    url = "file://" + url;
  }
  std::string escaped;
  escaped.reserve(url.size());
  for (char ch : url) {
    if (ch == '\\' || ch == '"') {
      escaped.push_back('\\');
    }
    escaped.push_back(ch);
  }
  return escaped;
}

std::string Appearance::BackgroundPositionCss(int position) {
  using Position = AppearanceSettings::BackgroundImagePosition;
  switch (static_cast<Position>(position)) {
    case Position::UpperLeft:
      return "top left";
    case Position::UpperRight:
      return "top right";
    case Position::Middle:
      return "center";
    case Position::BottomLeft:
      return "bottom left";
    case Position::BottomRight:
    default:
      return "bottom right";
  }
}

std::string Appearance::BackgroundCss(const std::string &override_path) const {
  using Type = AppearanceSettings::BackgroundImageType;
  const Type type = static_cast<Type>(background_type_);
  std::string path = override_path;
  if (type == Type::Custom || path.empty()) {
    path = background_filename_;
  }
  if (type == Type::Album && !override_path.empty()) {
    path = override_path;
  }
  return BuildBackgroundCss(background_type_, path, background_position_, blur_radius_, opacity_, background_stretch_,
                            background_keep_aspect_, background_do_not_cut_, background_max_size_);
}

std::string Appearance::BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity) {
  return BuildBackgroundCss(type, path, position, blur, opacity, AppearanceSettings::kDefaultBackgroundImageStretch,
                            AppearanceSettings::kDefaultBackgroundImageKeepAspectRatio, AppearanceSettings::kDefaultBackgroundImageDoNotCut,
                            AppearanceSettings::kDefaultBackgroundImageMaxSize);
}

std::string Appearance::BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity, bool stretch,
                                           bool keep_aspect, bool do_not_cut, int max_size) {
  using Type = AppearanceSettings::BackgroundImageType;
  const Type kind = static_cast<Type>(type);
  if (kind == Type::Default) {
    return {};
  }
  const std::string reset_main = std::string(kMainSelector) + " { background-image: none; filter: none; }";
  if (kind == Type::None) {
    return reset_main + kPlaylistViewportSelector + " { background-image: none; }";
  }
  if (kind == Type::Strawbs) {
    return reset_main + kPlaylistViewportSelector + " { background-color: #8B1E3F; background-image: none; }";
  }
  const std::string url = CssUrl(path);
  if (url.empty()) {
    return reset_main + kPlaylistViewportSelector + " { background-image: none; }";
  }
  const int level = std::clamp(opacity, 0, 100);
  const double veil = 1.0 - (static_cast<double>(level) / 100.0);
  const int radius = std::max(0, blur);
  std::string css = reset_main + kPlaylistViewportSelector + " { background-image: linear-gradient(rgba(0,0,0," + std::to_string(veil) +
                    "), rgba(0,0,0," + std::to_string(veil) + ")), url(\"" + url +
                    "\"); background-repeat: no-repeat; background-size: " + AppearanceColors::BackgroundSizeCss(stretch, keep_aspect, do_not_cut, max_size) +
                    "; background-position: " + BackgroundPositionCss(position) + ";";
  if (radius > 0) {
    css += " filter: blur(" + std::to_string(radius) + "px);";
  }
  css += " }";
  return css;
}
