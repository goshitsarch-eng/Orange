#include "context/contextview.h"

#include "constants/contextsettings.h"
#include "context/contexttechnical.h"
#include "core/settings.h"
#include "core/song.h"
#include "lyrics/lyricsfetcher.h"
#include "lyrics/lyricsproviders.h"
#include "translations/translations.h"
#include "utilities/styleutils.h"

ContextView::ContextView(LyricsProviders *lyrics_providers, LyricsFetcher *lyrics_fetcher)
    : lyrics_providers_(lyrics_providers), lyrics_fetcher_(lyrics_fetcher), album_(std::make_unique<ContextAlbum>()) {
  if (lyrics_fetcher_) {
    lyrics_fetcher_->LyricsFetched.Connect([this](uint64_t id, const std::string &, const std::string &lyrics) {
      if (id == current_search_id_) {
        SetLyrics(lyrics);
      }
    });
  }
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);

  GtkWidget *toggles = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  show_album_btn_ = gtk_toggle_button_new_with_label(Translations::CStr("Album"));
  show_data_btn_ = gtk_toggle_button_new_with_label(Translations::CStr("Details"));
  show_lyrics_btn_ = gtk_toggle_button_new_with_label(Translations::CStr("Lyrics"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_album_btn_), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_data_btn_), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_lyrics_btn_), TRUE);
  gtk_box_append(GTK_BOX(toggles), show_album_btn_);
  gtk_box_append(GTK_BOX(toggles), show_data_btn_);
  gtk_box_append(GTK_BOX(toggles), show_lyrics_btn_);

  title_ = gtk_label_new(Translations::CStr("Not playing"));
  gtk_widget_add_css_class(title_, "title-2");
  gtk_label_set_wrap(GTK_LABEL(title_), TRUE);
  artist_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_, "heading");
  album_label_ = gtk_label_new("");
  gtk_widget_add_css_class(album_label_, "dim-label");
  totals_ = gtk_label_new("");
  gtk_widget_add_css_class(totals_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(totals_), TRUE);
  gtk_label_set_justify(GTK_LABEL(totals_), GTK_JUSTIFY_CENTER);
  gtk_widget_set_visible(totals_, FALSE);
  data_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  data_grid_ = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(data_grid_), 4);
  gtk_grid_set_column_spacing(GTK_GRID(data_grid_), 12);
  gtk_box_append(GTK_BOX(data_box_), data_grid_);

  lyrics_view_ = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(lyrics_view_), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(lyrics_view_), FALSE);
  gtk_widget_set_vexpand(lyrics_view_, TRUE);
  GtkWidget *lyrics_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  search_lyrics_btn_ = gtk_button_new_with_label(Translations::CStr("Search lyrics"));
  auto_lyrics_btn_ = gtk_check_button_new_with_label(Translations::CStr("Search automatically"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(auto_lyrics_btn_), TRUE);
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save lyrics to tag"));
  gtk_box_append(GTK_BOX(lyrics_actions), search_lyrics_btn_);
  gtk_box_append(GTK_BOX(lyrics_actions), auto_lyrics_btn_);
  gtk_box_append(GTK_BOX(lyrics_actions), save);
  g_signal_connect(search_lyrics_btn_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<ContextView *>(data)->SearchLyrics(true);
                   }),
                   this);
  g_signal_connect(auto_lyrics_btn_, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     auto *self = static_cast<ContextView *>(data);
                     self->search_lyrics_ = gtk_check_button_get_active(button);
                     self->PersistVisibility();
                   }),
                   this);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<ContextView *>(data);
                     if (!self->save_lyrics_) {
                       return;
                     }
                     GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->lyrics_view_));
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buffer, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                     self->save_lyrics_(text ? text : "");
                     g_free(text);
                   }),
                   this);

  album_->SetDropCallback([this](const std::vector<unsigned char> &data) {
    if (cover_drop_) {
      cover_drop_(data);
    }
    AlbumCoverLoaded(data);
  });
  album_->SetFadeFinishedCallback([this]() { FadeStopFinished(); });

  gtk_box_append(GTK_BOX(box), toggles);
  gtk_box_append(GTK_BOX(box), album_->widget());
  gtk_box_append(GTK_BOX(box), title_);
  gtk_box_append(GTK_BOX(box), artist_);
  gtk_box_append(GTK_BOX(box), album_label_);
  gtk_box_append(GTK_BOX(box), totals_);
  gtk_box_append(GTK_BOX(box), data_box_);
  gtk_box_append(GTK_BOX(box), lyrics_view_);
  gtk_box_append(GTK_BOX(box), lyrics_actions);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), box);
  widget_ = scroll;

  auto notify = +[](GtkToggleButton *, gpointer data) { static_cast<ContextView *>(data)->ApplyVisibility(); };
  g_signal_connect(show_album_btn_, "toggled", G_CALLBACK(notify), this);
  g_signal_connect(show_data_btn_, "toggled", G_CALLBACK(notify), this);
  g_signal_connect(show_lyrics_btn_, "toggled", G_CALLBACK(notify), this);
}

void ContextView::SetSaveLyricsCallback(SaveLyricsCallback callback) { save_lyrics_ = std::move(callback); }

void ContextView::SetCoverDropCallback(CoverDropCallback callback) { cover_drop_ = std::move(callback); }

void ContextView::SetCollectionTotals(int songs, int artists, int albums) {
  totals_songs_ = songs;
  totals_artists_ = artists;
  totals_albums_ = albums;
  UpdateTotalsLabel();
}

void ContextView::UpdateTotalsLabel() {
  gtk_label_set_text(GTK_LABEL(totals_), ContextTechnical::Totals(totals_songs_, totals_artists_, totals_albums_).c_str());
}

void ContextView::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(ContextSettings::kSettingsGroup);
  show_album_ = settings.BoolValue(ContextSettings::kAlbum, ContextSettings::kDefaultAlbum);
  show_data_ = settings.BoolValue(ContextSettings::kTechnicalData, ContextSettings::kDefaultTechnicalData);
  show_lyrics_ = settings.BoolValue(ContextSettings::kSongLyrics, ContextSettings::kDefaultSongLyrics);
  search_lyrics_ = settings.BoolValue(ContextSettings::kSearchLyrics, ContextSettings::kDefaultSearchLyrics);
  title_fmt_ = settings.Value(ContextSettings::kSettingsTitleFmt, ContextSettings::kDefaultTitleFmt);
  summary_fmt_ = settings.Value(ContextSettings::kSettingsSummaryFmt, ContextSettings::kDefaultSummaryFmt);
  const std::string headline_font = settings.Value(ContextSettings::kFontHeadline, ContextSettings::kDefaultFontFamily);
  const std::string normal_font = settings.Value(ContextSettings::kFontNormal, ContextSettings::kDefaultFontFamily);
  const double headline_size = settings.DoubleValue(ContextSettings::kFontSizeHeadline, ContextSettings::kDefaultFontSizeHeadline);
  const double normal_size = settings.DoubleValue(ContextSettings::kFontSizeNormal, ContextSettings::kDefaultFontSizeNormal);
  if (title_) {
    gtk_widget_set_name(title_, "context-headline");
    StyleUtils::LoadCss("#context-headline { font-family: \"" + headline_font + "\"; font-size: " + std::to_string(headline_size) + "pt; }");
  }
  if (artist_ && album_label_) {
    gtk_widget_set_name(artist_, "context-normal");
    gtk_widget_set_name(album_label_, "context-summary");
    StyleUtils::LoadCss("#context-normal, #context-summary { font-family: \"" + normal_font + "\"; font-size: " +
                        std::to_string(normal_size) + "pt; }");
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_album_btn_), show_album_);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_data_btn_), show_data_);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_lyrics_btn_), show_lyrics_);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(auto_lyrics_btn_), search_lyrics_);
  ApplyVisibility();
}

void ContextView::Playing() { SetSong(); }

void ContextView::Stopped() {
  song_playing_ = Song();
  lyrics_tried_ = false;
  album_->SetImage({});
}

void ContextView::FadeStopFinished() {
  if (!song_playing_.is_valid() && song_playing_.url().empty()) {
    NoSong();
  }
}

void ContextView::Error() { gtk_label_set_text(GTK_LABEL(title_), Translations::CStr("Error")); }

void ContextView::NoSong() {
  song_playing_ = Song();
  lyrics_tried_ = false;
  gtk_label_set_text(GTK_LABEL(title_), Translations::CStr("Not playing"));
  gtk_label_set_text(GTK_LABEL(artist_), "");
  gtk_label_set_text(GTK_LABEL(album_label_), "");
  UpdateTotalsLabel();
  SetLyrics({});
  RebuildTechnicalData();
  ApplyVisibility();
}

void ContextView::SongChanged(const Song &song) {
  song_playing_ = song;
  lyrics_tried_ = false;
  SetSong();
  SearchLyrics(false);
}

void ContextView::SetSong() {
  const std::string headline = ContextTechnical::Headline(song_playing_, title_fmt_);
  gtk_label_set_text(GTK_LABEL(title_), headline.empty() ? Translations::CStr("Not playing") : headline.c_str());
  gtk_label_set_text(GTK_LABEL(artist_), song_playing_.artist().c_str());
  const std::string summary = ContextTechnical::Summary(song_playing_, summary_fmt_);
  gtk_label_set_text(GTK_LABEL(album_label_), summary.c_str());
  RebuildTechnicalData();
  ApplyVisibility();
}

void ContextView::RebuildTechnicalData() {
  GtkWidget *child = gtk_widget_get_first_child(data_grid_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_widget_unparent(child);
    child = next;
  }
  const auto rows = ContextTechnical::Rows(song_playing_);
  int row = 0;
  for (const auto &entry : rows) {
    GtkWidget *key = gtk_label_new(entry.first.c_str());
    gtk_widget_add_css_class(key, "dim-label");
    gtk_label_set_xalign(GTK_LABEL(key), 0.0f);
    GtkWidget *value = gtk_label_new(entry.second.c_str());
    gtk_label_set_xalign(GTK_LABEL(value), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(value), TRUE);
    gtk_grid_attach(GTK_GRID(data_grid_), key, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(data_grid_), value, 1, row, 1, 1);
    ++row;
  }
}

void ContextView::AlbumCoverLoaded(const std::vector<unsigned char> &data) { album_->SetImage(data); }

void ContextView::SetLyrics(const std::string &lyrics) {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(lyrics_view_));
  gtk_text_buffer_set_text(buffer, lyrics.empty() ? Translations::CStr("No lyrics") : lyrics.c_str(), -1);
}

void ContextView::SearchLyrics(bool force) {
  if (!force && !search_lyrics_) {
    return;
  }
  if (!force && lyrics_tried_) {
    return;
  }
  if (song_playing_.artist().empty() || song_playing_.title().empty()) {
    if (force) {
      SetLyrics({});
    }
    return;
  }
  lyrics_tried_ = true;
  if (lyrics_fetcher_) {
    current_search_id_ = lyrics_fetcher_->Search(song_playing_.EffectiveAlbumartist(), song_playing_.artist(), song_playing_.album(),
                                                 song_playing_.title(), song_playing_.length_nanosec());
    return;
  }
  if (!lyrics_providers_) {
    SetLyrics({});
    return;
  }
  lyrics_providers_->Fetch(song_playing_, [this](const std::string &lyrics, const std::string &) { SetLyrics(lyrics); });
}

void ContextView::PersistVisibility() {
  Settings settings;
  settings.BeginGroup(ContextSettings::kSettingsGroup);
  settings.SetBoolValue(ContextSettings::kAlbum, show_album_);
  settings.SetBoolValue(ContextSettings::kTechnicalData, show_data_);
  settings.SetBoolValue(ContextSettings::kSongLyrics, show_lyrics_);
  settings.SetBoolValue(ContextSettings::kSearchLyrics, search_lyrics_);
  settings.Sync();
}

void ContextView::ApplyVisibility() {
  show_album_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_album_btn_));
  show_data_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_data_btn_));
  show_lyrics_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_lyrics_btn_));
  const bool playing = song_playing_.is_valid() || !song_playing_.url().empty();
  gtk_widget_set_visible(album_->widget(), show_album_);
  gtk_widget_set_visible(totals_, !playing);
  gtk_widget_set_visible(data_box_, show_data_ && playing && gtk_widget_get_first_child(data_grid_) != nullptr);
  gtk_widget_set_visible(lyrics_view_, show_lyrics_);
  gtk_widget_set_visible(search_lyrics_btn_, show_lyrics_);
  gtk_widget_set_visible(auto_lyrics_btn_, show_lyrics_);
  PersistVisibility();
}
