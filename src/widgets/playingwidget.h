#ifndef STRAWBERRY_PLAYINGWIDGET_H
#define STRAWBERRY_PLAYINGWIDGET_H

#include "constants/filefilterconstants.h"
#include "core/signal.h"
#include "core/song.h"
#include "covermanager/coverchoicemenu.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

class PlayingWidget {
 public:
  enum class Mode { SmallSongDetails = 0, LargeSongDetails = 1 };
  using DropCallback = std::function<void(const std::vector<unsigned char> &)>;
  using CoverActionCallback = std::function<void(CoverChoiceMenu::Action)>;
  using SearchAutoChangedCallback = std::function<void(bool)>;

  static constexpr int kSmallCover = 48;
  static constexpr int kLargeCover = 160;
  static constexpr int kMinFitCover = 80;
  static constexpr int kMaxCoverSize = 260;
  static constexpr int kTopBorder = 4;
  static constexpr int kFadeTimelineMs = 1000;
  static constexpr int kFadeTickMs = 50;
  static constexpr int kShowHideMs = 500;

  PlayingWidget();
  ~PlayingWidget();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *cover() const { return cover_; }
  bool IsEnabled() const { return enabled_; }
  bool playing() const { return playing_; }
  bool active() const { return active_; }
  Mode mode() const { return mode_; }
  bool above_status_bar() const { return above_status_bar_; }
  bool fit_cover_width() const { return fit_cover_width_; }

  void SetEnabled(bool enabled);
  void SetMode(Mode mode);
  void SetAboveStatusBar(bool above);
  void SetFitCoverWidth(bool fit);
  void SetDropCallback(DropCallback callback);
  void SetCoverActionCallback(CoverActionCallback callback);
  void SetSearchAutoChangedCallback(SearchAutoChangedCallback callback) { search_auto_changed_ = std::move(callback); }
  void Playing();
  void Stopped();
  void Error();
  void SongChanged(const Song &song);
  void SetCover(const std::vector<unsigned char> &data);
  void SearchCoverInProgress();
  const Song &song() const { return song_; }

  Signal<bool> AboveStatusBarChanged;

  static std::string DetailsTitle(const Song &song) { return song.PrettyTitle(); }
  static std::string DetailsArtist(const Song &song) { return song.artist(); }
  static std::string DetailsAlbum(const Song &song) { return song.album(); }

  static bool ShouldShow(bool enabled, bool active) { return enabled && active; }

  // Qt PlayingWidget::SetMode: Fit cover width stays off in the small-cover layout.
  static bool FitCoverWidthEnabled(Mode mode) { return mode != Mode::SmallSongDetails; }

  static int DetailsEstimate(bool has_album) { return has_album ? 60 : 40; }

  static int LargeTotalHeight(int cover_size, int details_height) {
    return kTopBorder + std::max(0, cover_size) + std::max(0, details_height);
  }

  static int TotalHeight(Mode mode, int cover_size, int details_height) {
    if (mode == Mode::SmallSongDetails) {
      return std::max(kSmallCover, std::max(0, details_height));
    }
    return LargeTotalHeight(cover_size, details_height);
  }

  static int ShowHideElapsed(bool showing, int elapsed_ms, int tick_ms) {
    if (showing) {
      return std::min(kShowHideMs, elapsed_ms + std::max(0, tick_ms));
    }
    return std::max(0, elapsed_ms - std::max(0, tick_ms));
  }

  static bool ShowHideFinished(bool showing, int elapsed_ms) {
    return showing ? elapsed_ms >= kShowHideMs : elapsed_ms <= 0;
  }

  static double ShowHideProgress(int elapsed_ms) {
    if (elapsed_ms <= 0) {
      return 0.0;
    }
    if (elapsed_ms >= kShowHideMs) {
      return 1.0;
    }
    return static_cast<double>(elapsed_ms) / static_cast<double>(kShowHideMs);
  }

  static int AnimatedHeight(int total_height, int elapsed_ms) {
    return static_cast<int>(static_cast<double>(std::max(0, total_height)) * ShowHideProgress(elapsed_ms));
  }

  static int CoverSize(Mode mode, bool fit_width, int widget_width) {
    if (mode == Mode::SmallSongDetails) {
      return kSmallCover;
    }
    if (widget_width <= 0) {
      return fit_width ? kMaxCoverSize : kLargeCover;
    }
    if (fit_width) {
      return std::clamp(widget_width, kMinFitCover, kMaxCoverSize);
    }
    return std::min(kMaxCoverSize, widget_width);
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

  static bool IsImagePath(const std::string &path) { return FileFilterConstants::PathMatchesGlobs(path, FileFilterConstants::kLoadImages); }

 private:
  void SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data);
  void SnapshotCurrentToPrevious();
  void ApplyLayout();
  void ApplyVisibility(bool animate = true);
  int CurrentTotalHeight() const;
  void StartShowHide(bool show);
  void StopShowHide();
  void ApplyShowHideHeight();
  gboolean ShowHideTick();
  void LoadSettings();
  void SaveSettings() const;
  void ShowMenu(double x, double y);
  void StartFade();
  void StopFade();
  gboolean FadeTick();
  gboolean OnDrop(const GValue *value);

  GtkWidget *widget_ = nullptr;
  GtkWidget *previous_cover_ = nullptr;
  GtkWidget *cover_ = nullptr;
  GtkWidget *title_ = nullptr;
  GtkWidget *artist_ = nullptr;
  GtkWidget *album_ = nullptr;
  GtkWidget *labels_ = nullptr;
  GtkWidget *spinner_ = nullptr;
  DropCallback drop_;
  CoverActionCallback cover_action_;
  SearchAutoChangedCallback search_auto_changed_;
  bool enabled_ = true;
  bool playing_ = false;
  bool active_ = false;
  bool above_status_bar_ = false;
  bool fit_cover_width_ = false;
  Mode mode_ = Mode::LargeSongDetails;
  int fade_elapsed_ms_ = 0;
  guint fade_timeout_id_ = 0;
  bool shown_ = false;
  bool showhide_target_ = false;
  int showhide_elapsed_ms_ = 0;
  guint showhide_timeout_id_ = 0;
  Song song_;
};

#endif
