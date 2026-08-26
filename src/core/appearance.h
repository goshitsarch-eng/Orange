#ifndef STRAWBERRY_APPEARANCE_H
#define STRAWBERRY_APPEARANCE_H

#include "core/appearancecolors.h"

#include <map>
#include <string>

class Appearance {
 public:
  void ReloadSettings();
  void Apply();
  bool dark_mode() const { return dark_mode_; }
  bool system_icons() const { return system_icons_; }
  const std::string &style() const { return style_; }
  const std::string &background_filename() const { return background_filename_; }
  int background_type() const { return background_type_; }
  int background_position() const { return background_position_; }
  int blur_radius() const { return blur_radius_; }
  int opacity() const { return opacity_; }
  bool background_stretch() const { return background_stretch_; }
  AppearanceColors::IconSizes icon_sizes() const { return icon_sizes_; }
  std::string BackgroundCss(const std::string &override_path = {}) const;
  std::string ThemeCss() const;
  static std::string BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity);
  static std::string BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity, bool stretch,
                                        bool keep_aspect, bool do_not_cut, int max_size);
  static std::string BackgroundPositionCss(int position);
  static std::string CssUrl(const std::string &path);

 private:
  bool dark_mode_ = false;
  bool system_icons_ = true;
  bool use_custom_colors_ = false;
  bool tabbar_system_color_ = false;
  bool tabbar_gradient_ = true;
  bool background_stretch_ = false;
  bool background_keep_aspect_ = true;
  bool background_do_not_cut_ = true;
  std::string style_;
  std::string background_filename_;
  std::string tabbar_color_;
  std::string playing_song_color_;
  std::map<std::string, std::string> custom_colors_;
  AppearanceColors::IconSizes icon_sizes_;
  int background_type_ = 0;
  int background_position_ = 5;
  int blur_radius_ = 0;
  int opacity_ = 100;
  int background_max_size_ = 0;
};

#endif
