#include "core/appearance.h"

#include "constants/appearancesettings.h"
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
  background_filename_ = settings.Value(AppearanceSettings::kBackgroundImageFilename);
  background_type_ = settings.IntValue(AppearanceSettings::kBackgroundImageType, static_cast<int>(AppearanceSettings::kDefaultBackgroundImageType));
  background_position_ = settings.IntValue(AppearanceSettings::kBackgroundImagePosition,
                                           static_cast<int>(AppearanceSettings::kDefaultBackgroundImagePosition));
  blur_radius_ = settings.IntValue(AppearanceSettings::kBackgroundImageBlurRadius, AppearanceSettings::kDefaultBackgroundImageBlurRadius);
  opacity_ = settings.IntValue(AppearanceSettings::kBackgroundImageOpacityLevel, AppearanceSettings::kDefaultBackgroundImageOpacityLevel);
}

void Appearance::Apply() {
  ReloadSettings();
  AdwStyleManager *manager = adw_style_manager_get_default();
  if (manager) {
    adw_style_manager_set_color_scheme(manager, dark_mode_ ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
  }
  if (!style_.empty()) {
    StyleUtils::LoadCss(style_);
  }
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
  return BuildBackgroundCss(background_type_, path, background_position_, blur_radius_, opacity_);
}

std::string Appearance::BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity) {
  using Type = AppearanceSettings::BackgroundImageType;
  const Type kind = static_cast<Type>(type);
  if (kind == Type::Default) {
    return {};
  }
  if (kind == Type::None) {
    return ".strawberry-main { background-image: none; }";
  }
  if (kind == Type::Strawbs) {
    return ".strawberry-main { background-color: #8B1E3F; background-image: none; }";
  }
  const std::string url = CssUrl(path);
  if (url.empty()) {
    return ".strawberry-main { background-image: none; }";
  }
  const int level = std::clamp(opacity, 0, 100);
  const double veil = 1.0 - (static_cast<double>(level) / 100.0);
  const int radius = std::max(0, blur);
  std::string css = ".strawberry-main { background-image: linear-gradient(rgba(0,0,0," + std::to_string(veil) + "), rgba(0,0,0," +
                    std::to_string(veil) + ")), url(\"" + url + "\"); background-size: cover; background-position: " +
                    BackgroundPositionCss(position) + ";";
  if (radius > 0) {
    css += " filter: blur(" + std::to_string(radius) + "px);";
  }
  css += " }";
  return css;
}
