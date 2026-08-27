#ifndef STRAWBERRY_OSDPRETTY_H
#define STRAWBERRY_OSDPRETTY_H

#include <gtk/gtk.h>

#include <string>
#include <utility>
#include <vector>

class OSDPretty {
 public:
  enum class Mode { Popup, Draggable };

  explicit OSDPretty(Mode mode = Mode::Popup);
  ~OSDPretty();

  void ReloadSettings();
  void SetMessage(const std::string &summary, const std::string &message, const std::vector<unsigned char> &image = {});
  void ShowMessage(const std::string &summary, const std::string &message, const std::vector<unsigned char> &image = {});
  void set_popup_duration(int msec) { timeout_ms_ = msec; }
  void set_toggle_mode(bool toggle) { toggle_mode_ = toggle; }
  bool toggle_mode() const { return toggle_mode_; }
  void set_foreground_color(const std::string &color) { fg_ = color; }
  void set_background_color(const std::string &color) { bg_ = color; }
  void set_background_opacity(double opacity) { opacity_ = opacity; }
  void set_font(const std::string &font) { font_ = font; }
  int pos_x() const { return pos_x_; }
  int pos_y() const { return pos_y_; }
  void set_pos(int x, int y);
  void SavePosition() const;
  bool IsTransparencyAvailable() const;
  static bool Supported();
  static std::vector<std::pair<std::string, std::string>> MonitorChoices();
  bool fading() const { return fading_; }
  bool disable_duration() const { return disable_duration_; }
  const std::string &popup_screen() const { return popup_screen_; }
  GtkWidget *window() const { return window_; }

 private:
  void EnsureWindow();
  void ApplyStyle();
  void ApplyPosition();
  void ApplyShape();
  void ApplyLimits();
  void ConnectDrag();
  void ConnectPopup();
  void StopFade();
  void StartFade(bool fading_in);
  void StartHideTimeout();
  void HideNow();
  void SetHoverDim(bool dimmed);
  void SetSnapHighlight(bool snapped);
  static void OnDragBegin(GtkGestureDrag *gesture, double x, double y, gpointer data);
  static void OnDragUpdate(GtkGestureDrag *gesture, double x, double y, gpointer data);
  static void OnDragEnd(GtkGestureDrag *gesture, double x, double y, gpointer data);

  Mode mode_ = Mode::Popup;
  int timeout_ms_ = 4000;
  std::string fg_ = "#ffffff";
  std::string bg_ = "#202020";
  double opacity_ = 0.92;
  int pos_x_ = 40;
  int pos_y_ = 40;
  std::string font_ = "Sans 12";
  std::string popup_screen_;
  bool show_art_ = true;
  bool fading_ = true;
  bool disable_duration_ = false;
  GtkWidget *window_ = nullptr;
  GtkWidget *title_ = nullptr;
  GtkWidget *body_ = nullptr;
  GtkWidget *image_ = nullptr;
  guint timeout_id_ = 0;
  guint fade_id_ = 0;
  double drag_start_x_ = 0;
  double drag_start_y_ = 0;
  bool toggle_mode_ = false;
  bool hover_dimmed_ = false;
  bool snap_highlight_ = false;
};

#endif
