#ifndef STRAWBERRY_FANCYTABBAR_H
#define STRAWBERRY_FANCYTABBAR_H

#include "widgets/fancytabdata.h"
#include "widgets/fancytabmode.h"

#include <functional>
#include <gtk/gtk.h>
#include <string>
#include <vector>

class FancyTabBar {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using ModeChangedCallback = std::function<void(FancyTabMode::Mode)>;

  FancyTabBar();

  GtkWidget *widget() const { return widget_; }
  FancyTabMode::Mode mode() const { return mode_; }
  const std::string &active() const { return active_; }
  int ActiveIndex() const;

  void AddTab(const std::string &id, const std::string &title, const std::string &icon);
  void SetActive(const std::string &id, bool notify = true);
  void SetActiveIndex(int index, bool notify = true);
  void SetMode(FancyTabMode::Mode mode);
  void ReloadIconSizes();
  void SetActivateCallback(ActivateCallback callback);
  void SetModeChangedCallback(ModeChangedCallback callback);
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);

 private:
  void Rebuild();
  void ShowModeMenu();
  void UpdateToggles() const;
  int IconPixels() const;

  GtkWidget *widget_ = nullptr;
  FancyTabMode::Mode mode_ = FancyTabMode::kDefaultMode;
  std::string active_;
  std::vector<FancyTabData> tabs_;
  ActivateCallback activate_;
  ModeChangedCallback mode_changed_;
  int icon_large_ = FancyTabMode::kLargeIcon;
  int icon_small_ = FancyTabMode::kSmallIcon;
};

#endif
