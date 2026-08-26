#include "context/contextview.h"

#include "core/song.h"
#include "lyrics/lyricsfetcher.h"
#include "lyrics/lyricsproviders.h"
#include "utilities/timeutils.h"

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
  show_album_btn_ = gtk_toggle_button_new_with_label("Album");
  show_data_btn_ = gtk_toggle_button_new_with_label("Details");
  show_lyrics_btn_ = gtk_toggle_button_new_with_label("Lyrics");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_album_btn_), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_data_btn_), TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_lyrics_btn_), TRUE);
  gtk_box_append(GTK_BOX(toggles), show_album_btn_);
  gtk_box_append(GTK_BOX(toggles), show_data_btn_);
  gtk_box_append(GTK_BOX(toggles), show_lyrics_btn_);

  title_ = gtk_label_new("Not playing");
  gtk_widget_add_css_class(title_, "title-2");
  gtk_label_set_wrap(GTK_LABEL(title_), TRUE);
  artist_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_, "heading");
  album_label_ = gtk_label_new("");
  gtk_widget_add_css_class(album_label_, "dim-label");
  meta_ = gtk_label_new("");
  gtk_widget_add_css_class(meta_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(meta_), TRUE);
  data_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(data_box_), meta_);

  lyrics_view_ = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(lyrics_view_), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(lyrics_view_), FALSE);
  gtk_widget_set_vexpand(lyrics_view_, TRUE);
  GtkWidget *save = gtk_button_new_with_label("Save lyrics to tag");
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

  gtk_box_append(GTK_BOX(box), toggles);
  gtk_box_append(GTK_BOX(box), album_->widget());
  gtk_box_append(GTK_BOX(box), title_);
  gtk_box_append(GTK_BOX(box), artist_);
  gtk_box_append(GTK_BOX(box), album_label_);
  gtk_box_append(GTK_BOX(box), data_box_);
  gtk_box_append(GTK_BOX(box), lyrics_view_);
  gtk_box_append(GTK_BOX(box), save);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), box);
  widget_ = scroll;

  auto notify = +[](GtkToggleButton *, gpointer data) { static_cast<ContextView *>(data)->ApplyVisibility(); };
  g_signal_connect(show_album_btn_, "toggled", G_CALLBACK(notify), this);
  g_signal_connect(show_data_btn_, "toggled", G_CALLBACK(notify), this);
  g_signal_connect(show_lyrics_btn_, "toggled", G_CALLBACK(notify), this);
}

void ContextView::SetSaveLyricsCallback(SaveLyricsCallback callback) { save_lyrics_ = std::move(callback); }

void ContextView::ReloadSettings() {}

void ContextView::Playing() { SetSong(); }

void ContextView::Stopped() { NoSong(); }

void ContextView::Error() { gtk_label_set_text(GTK_LABEL(title_), "Error"); }

void ContextView::NoSong() {
  song_playing_ = Song();
  gtk_label_set_text(GTK_LABEL(title_), "Not playing");
  gtk_label_set_text(GTK_LABEL(artist_), "");
  gtk_label_set_text(GTK_LABEL(album_label_), "");
  gtk_label_set_text(GTK_LABEL(meta_), "");
  album_->Clear();
  SetLyrics({});
}

void ContextView::SongChanged(const Song &song) {
  song_playing_ = song;
  SetSong();
  SearchLyrics();
}

void ContextView::SetSong() {
  gtk_label_set_text(GTK_LABEL(title_), song_playing_.PrettyTitle().empty() ? "Not playing" : song_playing_.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(artist_), song_playing_.artist().c_str());
  gtk_label_set_text(GTK_LABEL(album_label_), song_playing_.album().c_str());
  const std::string meta = Song::SourceToString(song_playing_.source()) + " · " +
                           (song_playing_.bitrate() > 0 ? std::to_string(song_playing_.bitrate()) + " kbps · " : "") +
                           (song_playing_.samplerate() > 0 ? std::to_string(song_playing_.samplerate()) + " Hz · " : "") +
                           (song_playing_.bitdepth() > 0 ? std::to_string(song_playing_.bitdepth()) + "-bit · " : "") +
                           Utilities::PrettyTimeNanosec(song_playing_.length_nanosec());
  gtk_label_set_text(GTK_LABEL(meta_), meta.c_str());
}

void ContextView::AlbumCoverLoaded(const std::vector<unsigned char> &data) { album_->SetImage(data); }

void ContextView::SetLyrics(const std::string &lyrics) {
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(lyrics_view_));
  gtk_text_buffer_set_text(buffer, lyrics.empty() ? "No lyrics" : lyrics.c_str(), -1);
}

void ContextView::SearchLyrics() {
  if (!song_playing_.is_valid()) {
    SetLyrics({});
    return;
  }
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

void ContextView::ApplyVisibility() {
  show_album_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_album_btn_));
  show_data_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_data_btn_));
  show_lyrics_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_lyrics_btn_));
  gtk_widget_set_visible(album_->widget(), show_album_);
  gtk_widget_set_visible(data_box_, show_data_);
  gtk_widget_set_visible(lyrics_view_, show_lyrics_);
}
