#ifndef STRAWBERRY_CONTEXTVIEW_H
#define STRAWBERRY_CONTEXTVIEW_H

#include "context/contextalbum.h"
#include "core/song.h"
#include "lyrics/lrcparser.h"

#include <gtk/gtk.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class LyricsFetcher;
class LyricsProviders;

class ContextView {
 public:
  using SaveLyricsCallback = std::function<void(const std::string &)>;
  using CoverDropCallback = std::function<void(const std::vector<unsigned char> &)>;

  explicit ContextView(LyricsProviders *lyrics_providers = nullptr, LyricsFetcher *lyrics_fetcher = nullptr);

  GtkWidget *widget() const { return widget_; }
  ContextAlbum *album_widget() { return album_.get(); }
  bool album_enabled() const { return show_album_; }
  bool search_cover_enabled() const { return search_cover_; }
  const Song &song_playing() const { return song_playing_; }

  void Playing();
  void Stopped();
  void Error();
  void SongChanged(const Song &song);
  void AlbumCoverLoaded(const std::vector<unsigned char> &data);
  void SetLyrics(const std::string &lyrics, const std::string &provider = {});
  void SetPlaybackPosition(int64_t position_nanosec);
  void SearchLyrics(bool force = false);
  void SetSaveLyricsCallback(SaveLyricsCallback callback);
  void SetCoverDropCallback(CoverDropCallback callback);
  void SetCollectionTotals(int songs, int artists, int albums);
  void ReloadSettings();

 private:
  void NoSong();
  void SetSong();
  void FadeStopFinished();
  void ApplyVisibility();
  void PersistVisibility();
  void RebuildTechnicalData();
  void UpdateTotalsLabel();
  void HighlightLrcLine(int index);

  LyricsProviders *lyrics_providers_ = nullptr;
  LyricsFetcher *lyrics_fetcher_ = nullptr;
  uint64_t current_search_id_ = 0;
  std::unique_ptr<ContextAlbum> album_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *title_ = nullptr;
  GtkWidget *artist_ = nullptr;
  GtkWidget *album_label_ = nullptr;
  GtkWidget *totals_ = nullptr;
  GtkWidget *data_box_ = nullptr;
  GtkWidget *data_grid_ = nullptr;
  GtkWidget *lyrics_view_ = nullptr;
  GtkWidget *lyrics_source_ = nullptr;
  GtkWidget *search_lyrics_btn_ = nullptr;
  GtkWidget *auto_lyrics_btn_ = nullptr;
  GtkWidget *auto_cover_btn_ = nullptr;
  GtkWidget *show_album_btn_ = nullptr;
  GtkWidget *show_data_btn_ = nullptr;
  GtkWidget *show_lyrics_btn_ = nullptr;
  Song song_playing_;
  bool show_album_ = true;
  bool show_data_ = true;
  bool show_lyrics_ = true;
  bool search_lyrics_ = true;
  bool search_cover_ = true;
  bool lyrics_tried_ = false;
  int totals_songs_ = 0;
  int totals_artists_ = 0;
  int totals_albums_ = 0;
  std::string title_fmt_;
  std::string summary_fmt_;
  SaveLyricsCallback save_lyrics_;
  CoverDropCallback cover_drop_;
  std::vector<LrcParser::Line> lrc_lines_;
  int lrc_active_ = -1;
  GtkTextTag *lrc_tag_ = nullptr;
};

#endif
