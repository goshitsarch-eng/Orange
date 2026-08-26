#ifndef STRAWBERRY_DYNAMICPLAYLISTCONTROLS_H
#define STRAWBERRY_DYNAMICPLAYLISTCONTROLS_H

#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

#include <functional>

class DynamicPlaylistControls {
 public:
  using ExpandCallback = std::function<void()>;
  using RepopulateCallback = std::function<void()>;
  using TurnOffCallback = std::function<void()>;

  DynamicPlaylistControls();

  GtkWidget *widget() const { return widget_; }
  void SetSearch(const SmartPlaylistSearch &search);
  const SmartPlaylistSearch &search() const { return search_; }
  void SetExpandCallback(ExpandCallback callback);
  void SetRepopulateCallback(RepopulateCallback callback);
  void SetTurnOffCallback(TurnOffCallback callback);
  void SetVisible(bool visible);

 private:
  GtkWidget *widget_ = nullptr;
  SmartPlaylistSearch search_;
  ExpandCallback expand_;
  RepopulateCallback repopulate_;
  TurnOffCallback turn_off_;
};

#endif
