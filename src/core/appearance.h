#ifndef STRAWBERRY_APPEARANCE_H
#define STRAWBERRY_APPEARANCE_H

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
  std::string BackgroundCss(const std::string &override_path = {}) const;
  static std::string BuildBackgroundCss(int type, const std::string &path, int position, int blur, int opacity);
  static std::string BackgroundPositionCss(int position);
  static std::string CssUrl(const std::string &path);

 private:
  bool dark_mode_ = false;
  bool system_icons_ = true;
  std::string style_;
  std::string background_filename_;
  int background_type_ = 0;
  int background_position_ = 5;
  int blur_radius_ = 0;
  int opacity_ = 100;
};

#endif
