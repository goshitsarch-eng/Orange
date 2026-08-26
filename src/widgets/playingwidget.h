#ifndef STRAWBERRY_PLAYINGWIDGET_H
#define STRAWBERRY_PLAYINGWIDGET_H

#include "core/signal.h"
#include "core/song.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <vector>

class PlayingWidget {
 public:
  enum class Mode { SmallSongDetails = 0, LargeSongDetails = 1 };

  static constexpr int kSmallCover = 48;
  static constexpr int kLargeCover = 160;
  static constexpr int kMinFitCover = 80;
  static constexpr int kMaxFitCover = 320;
  static constexpr int kFadeTimelineMs = 1000;

  PlayingWidget();
  ~PlayingWidget();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *cover() const { return cover_; }
  bool IsEnabled() const { return enabled_; }
  bool playing() const { return playing_; }
  Mode mode() const { return mode_; }
  bool above_status_bar() const { return above_status_bar_; }
  bool fit_cover_width() const { return fit_cover_width_; }

  void SetEnabled(bool enabled);
  void SetMode(Mode mode);
  void SetAboveStatusBar(bool above);
  void SetFitCoverWidth(bool fit);
  void Playing();
  void Stopped();
  void Error();
  void SongChanged(const Song &song);
  void SetCover(const std::vector<unsigned char> &data);
  void SearchCoverInProgress();
  const Song &song() const { return song_; }

  Signal<bool> AboveStatusBarChanged;

  static int CoverSize(Mode mode, bool fit_width, int widget_width) {
    if (mode == Mode::SmallSongDetails) {
      return kSmallCover;
    }
    if (fit_width && widget_width > 0) {
      return std::clamp(widget_width, kMinFitCover, kMaxFitCover);
    }
    return kLargeCover;
  }

  static double FadeInOpacity(int elapsed_ms) {
    if (elapsed_ms <= 0) {
      return 0.0;
    }
    if (elapsed_ms >= kFadeTimelineMs) {
      return 1.0;
    }
    return static_cast<double>(elapsed_ms) / static_cast<double>(kFadeTimelineMs);
  }

  static double FadeOutOpacity(int elapsed_ms) { return 1.0 - FadeInOpacity(elapsed_ms); }

 private:
  void SetImageFromBytes(const std::vector<unsigned char> &data);
  void ApplyLayout();
  void LoadSettings();
  void SaveSettings() const;
  void ShowMenu(double x, double y);
  void StartFade();
  void StopFade();

  GtkWidget *widget_ = nullptr;
  GtkWidget *cover_ = nullptr;
  GtkWidget *title_ = nullptr;
  GtkWidget *artist_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  bool enabled_ = true;
  bool playing_ = false;
  bool above_status_bar_ = false;
  bool fit_cover_width_ = false;
  Mode mode_ = Mode::LargeSongDetails;
  int fade_elapsed_ms_ = 0;
  guint fade_timeout_id_ = 0;
  Song song_;
};

#endif
