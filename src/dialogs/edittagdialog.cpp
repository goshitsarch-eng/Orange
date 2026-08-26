#include "dialogs/edittagdialog.h"

#include "core/application.h"
#include "covermanager/albumcoverchoicecontroller.h"
#include "dialogs/dialoghelpers.h"
#include "utilities/fileutils.h"
#include "utilities/timeutils.h"

#include <adwaita.h>
#include <cstdlib>
#include <memory>
#include <utility>

using DialogHelpers::PrettyBytes;
using DialogHelpers::PrettyUnixTime;
using DialogHelpers::SetImageFromBytes;
using DialogHelpers::SongForDialog;

void EditTagDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  SongList targets = songs;
  if (targets.empty()) {
    targets.push_back(SongForDialog(app));
  }
  const Song song = targets.front();
  auto common = [&](const auto &getter) -> std::pair<std::string, bool> {
    std::string value = getter(targets.front());
    bool mixed = false;
    for (const Song &item : targets) {
      if (getter(item) != value) {
        mixed = true;
        value.clear();
        break;
      }
    }
    return {value, mixed};
  };
  AdwDialog *dialog = adw_dialog_new();
  const std::string dialog_title = targets.size() > 1 ? "Edit tags (" + std::to_string(targets.size()) + " songs)" : "Edit tags";
  adw_dialog_set_title(dialog, dialog_title.c_str());
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 720);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  AdwViewStack *stack = ADW_VIEW_STACK(adw_view_stack_new());
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), stack);

  struct State {
    Song song;
    SongList songs;
    GtkWidget *cover = nullptr;
    GtkWidget *lyrics = nullptr;
    GtkWidget *rating = nullptr;
    GtkWidget *compilation = nullptr;
    std::unique_ptr<AlbumCoverChoiceController> covers;
    std::vector<std::pair<std::string, GtkWidget *>> fields;
    std::vector<std::string> initial;
  };
  auto *state = new State();
  state->song = song;
  state->songs = targets;
  state->covers = std::make_unique<AlbumCoverChoiceController>(app);

  auto add_entries = [&](GtkWidget *page, const std::vector<std::pair<const char *, std::pair<std::string, bool>>> &rows) {
    for (const auto &row : rows) {
      AdwEntryRow *entry = ADW_ENTRY_ROW(adw_entry_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), row.first);
      gtk_editable_set_text(GTK_EDITABLE(entry), row.second.first.c_str());
      if (row.second.second) {
        gtk_widget_set_tooltip_text(GTK_WIDGET(entry), "Multiple values — type to set all selected songs");
      }
      state->fields.emplace_back(row.first, GTK_WIDGET(entry));
      state->initial.push_back(row.second.first);
      gtk_box_append(GTK_BOX(page), GTK_WIDGET(entry));
    }
  };

  GtkWidget *summary = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(summary, 12);
  gtk_widget_set_margin_end(summary, 12);
  gtk_widget_set_margin_top(summary, 12);
  state->cover = gtk_image_new();
  gtk_widget_set_halign(state->cover, GTK_ALIGN_CENTER);
  SetImageFromBytes(state->cover, app->albumcover_loader()->LoadData(song), 160);
  gtk_box_append(GTK_BOX(summary), state->cover);
  GtkWidget *cover_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(cover_buttons, GTK_ALIGN_CENTER);
  GtkWidget *fetch_cover = gtk_button_new_with_label("Fetch cover");
  GtkWidget *search_cover = gtk_button_new_with_label("Search…");
  GtkWidget *url_cover = gtk_button_new_with_label("From URL");
  GtkWidget *file_cover = gtk_button_new_with_label("From file");
  GtkWidget *unset_cover = gtk_button_new_with_label("Unset");
  GtkWidget *stats_cover = gtk_button_new_with_label("Statistics");
  gtk_box_append(GTK_BOX(cover_buttons), fetch_cover);
  gtk_box_append(GTK_BOX(cover_buttons), search_cover);
  gtk_box_append(GTK_BOX(cover_buttons), url_cover);
  gtk_box_append(GTK_BOX(cover_buttons), file_cover);
  gtk_box_append(GTK_BOX(cover_buttons), unset_cover);
  gtk_box_append(GTK_BOX(cover_buttons), stats_cover);
  gtk_box_append(GTK_BOX(summary), cover_buttons);
  const std::string stats = "Plays: " + std::to_string(song.playcount()) + "   Skips: " + std::to_string(song.skipcount()) +
                            "   Last played: " + PrettyUnixTime(song.lastplayed()) + "\n" +
                            FileUtils::PathFromUri(song.url()) + "\n" + PrettyBytes(song.filesize()) + " · " +
                            Utilities::PrettyTimeNanosec(song.length_nanosec()) + " · " +
                            (song.bitrate() > 0 ? std::to_string(song.bitrate()) + " kbps" : "") +
                            (song.samplerate() > 0 ? " · " + std::to_string(song.samplerate()) + " Hz" : "") +
                            (song.bitdepth() > 0 ? " · " + std::to_string(song.bitdepth()) + "-bit" : "") + " · " +
                            Song::FiletypeToString(song.filetype());
  GtkWidget *stats_label = gtk_label_new(stats.c_str());
  gtk_label_set_wrap(GTK_LABEL(stats_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(stats_label), 0);
  gtk_widget_add_css_class(stats_label, "dim-label");
  gtk_box_append(GTK_BOX(summary), stats_label);
  GtkWidget *rating_label = gtk_label_new("Rating");
  gtk_label_set_xalign(GTK_LABEL(rating_label), 0);
  gtk_box_append(GTK_BOX(summary), rating_label);
  state->rating = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 5, 0.5);
  gtk_scale_set_digits(GTK_SCALE(state->rating), 1);
  gtk_scale_set_draw_value(GTK_SCALE(state->rating), TRUE);
  gtk_range_set_value(GTK_RANGE(state->rating), song.rating() >= 0 ? song.rating() * 5.0 : 0);
  gtk_box_append(GTK_BOX(summary), state->rating);
  add_entries(summary, {{"Title", common([](const Song &s) { return s.title(); })},
                        {"Artist", common([](const Song &s) { return s.artist(); })},
                        {"Album", common([](const Song &s) { return s.album(); })},
                        {"Album artist", common([](const Song &s) { return s.albumartist(); })},
                        {"Year", common([](const Song &s) { return s.year() > 0 ? std::to_string(s.year()) : std::string(); })},
                        {"Original year", common([](const Song &s) { return s.originalyear() > 0 ? std::to_string(s.originalyear()) : std::string(); })},
                        {"Track", common([](const Song &s) { return s.track() > 0 ? std::to_string(s.track()) : std::string(); })},
                        {"Genre", common([](const Song &s) { return s.genre(); })}});
  adw_view_stack_add_titled(stack, summary, "Summary", "Summary");

  GtkWidget *tags = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(tags, 12);
  gtk_widget_set_margin_end(tags, 12);
  gtk_widget_set_margin_top(tags, 12);
  state->compilation = gtk_check_button_new_with_label("Compilation");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->compilation), song.compilation());
  gtk_box_append(GTK_BOX(tags), state->compilation);
  add_entries(tags, {{"Composer", common([](const Song &s) { return s.composer(); })},
                     {"Performer", common([](const Song &s) { return s.performer(); })},
                     {"Grouping", common([](const Song &s) { return s.grouping(); })},
                     {"Comment", common([](const Song &s) { return s.comment(); })},
                     {"Disc", common([](const Song &s) { return s.disc() > 0 ? std::to_string(s.disc()) : std::string(); })},
                     {"BPM", common([](const Song &s) { return s.bpm() > 0 ? std::to_string(s.bpm()) : std::string(); })},
                     {"Mood", common([](const Song &s) { return s.mood(); })},
                     {"Initial key", common([](const Song &s) { return s.initial_key(); })},
                     {"Title sort", common([](const Song &s) { return s.titlesort(); })},
                     {"Artist sort", common([](const Song &s) { return s.artistsort(); })},
                     {"Album sort", common([](const Song &s) { return s.albumsort(); })},
                     {"Album artist sort", common([](const Song &s) { return s.albumartistsort(); })}});
  adw_view_stack_add_titled(stack, tags, "Tags", "Tags");

  GtkWidget *lyrics_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(lyrics_page, 12);
  gtk_widget_set_margin_end(lyrics_page, 12);
  gtk_widget_set_margin_top(lyrics_page, 12);
  state->lyrics = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->lyrics), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(state->lyrics, TRUE);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), song.lyrics().c_str(), -1);
  state->fields.emplace_back("Lyrics", state->lyrics);
  state->initial.push_back(song.lyrics());
  gtk_box_append(GTK_BOX(lyrics_page), gtk_label_new("Lyrics"));
  gtk_box_append(GTK_BOX(lyrics_page), state->lyrics);
  adw_view_stack_add_titled(stack, lyrics_page, "Lyrics", "Lyrics");

  GtkWidget *save = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(save, "suggested-action");
  g_object_set_data_full(G_OBJECT(save), "state", state, [](gpointer p) { delete static_cast<State *>(p); });
  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     auto text_of = [](GtkWidget *widget) -> std::string {
                       if (GTK_IS_TEXT_VIEW(widget)) {
                         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                         GtkTextIter start;
                         GtkTextIter end;
                         gtk_text_buffer_get_bounds(buffer, &start, &end);
                         gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                         std::string result = text ? text : "";
                         g_free(text);
                         return result;
                       }
                       return gtk_editable_get_text(GTK_EDITABLE(widget));
                     };
                     auto apply_field = [](Song *song, const std::string &name, const std::string &value) {
                       if (name == "Title") song->set_title(value);
                       else if (name == "Artist") song->set_artist(value);
                       else if (name == "Album") song->set_album(value);
                       else if (name == "Album artist") song->set_albumartist(value);
                       else if (name == "Composer") song->set_composer(value);
                       else if (name == "Performer") song->set_performer(value);
                       else if (name == "Grouping") song->set_grouping(value);
                       else if (name == "Comment") song->set_comment(value);
                       else if (name == "Genre") song->set_genre(value);
                       else if (name == "Lyrics") song->set_lyrics(value);
                       else if (name == "Mood") song->set_mood(value);
                       else if (name == "Initial key") song->set_initial_key(value);
                       else if (name == "Title sort") song->set_titlesort(value);
                       else if (name == "Artist sort") song->set_artistsort(value);
                       else if (name == "Album sort") song->set_albumsort(value);
                       else if (name == "Album artist sort") song->set_albumartistsort(value);
                       else if (name == "Year") song->set_year(std::atoi(value.c_str()));
                       else if (name == "Original year") song->set_originalyear(std::atoi(value.c_str()));
                       else if (name == "Track") song->set_track(std::atoi(value.c_str()));
                       else if (name == "Disc") song->set_disc(std::atoi(value.c_str()));
                       else if (name == "BPM") song->set_bpm(std::strtof(value.c_str(), nullptr));
                     };
                     std::vector<std::pair<std::string, std::string>> changed;
                     for (size_t i = 0; i < state->fields.size(); ++i) {
                       const std::string value = text_of(state->fields[i].second);
                       const std::string initial = i < state->initial.size() ? state->initial[i] : std::string();
                       if (value == initial) {
                         continue;
                       }
                       changed.emplace_back(state->fields[i].first, value);
                     }
                     const bool write_compilation = state->compilation != nullptr;
                     const bool write_rating = state->rating != nullptr;
                     for (Song &song : state->songs) {
                       for (const auto &field : changed) {
                         apply_field(&song, field.first, field.second);
                       }
                       if (write_compilation) {
                         song.set_compilation(gtk_check_button_get_active(GTK_CHECK_BUTTON(state->compilation)));
                       }
                       if (write_rating) {
                         song.set_rating(static_cast<float>(gtk_range_get_value(GTK_RANGE(state->rating)) / 5.0));
                       }
                       application->tagreader()->WriteFile(song);
                       const std::string path = FileUtils::PathFromUri(song.url());
                       if (!path.empty() && song.rating() >= 0) {
                         application->tagreader()->SaveRating(path, song.rating());
                       }
                       if (song.id() > 0) {
                         application->collection()->backend()->AddOrUpdateSong(song);
                         application->collection()->backend()->SetRating(song.id(), song.rating());
                       }
                     }
                     if (!state->songs.empty()) {
                       state->song = state->songs.front();
                     }
                     gtk_button_set_label(button, "Saved");
                   })),
                   app);

  g_object_set_data(G_OBJECT(fetch_cover), "state", state);
  g_signal_connect(fetch_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->FetchCover(&state->song, state->cover, GTK_WIDGET(button));
                   }),
                   nullptr);
  g_signal_connect(search_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->SearchForCover(GTK_WINDOW(data));
                   }),
                   parent);
  g_object_set_data(G_OBJECT(url_cover), "state", state);
  g_signal_connect(url_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->LoadCoverFromURL(GTK_WINDOW(data), &state->song, state->cover);
                   }),
                   parent);
  g_object_set_data(G_OBJECT(file_cover), "state", state);
  g_signal_connect(file_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->LoadCoverFromFile(GTK_WINDOW(data), &state->song, state->cover);
                   }),
                   parent);
  g_object_set_data(G_OBJECT(unset_cover), "state", state);
  g_signal_connect(unset_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->UnsetCover(&state->song, state->cover);
                     gtk_button_set_label(button, "Unset");
                   }),
                   nullptr);
  g_object_set_data(G_OBJECT(stats_cover), "state", state);
  g_signal_connect(stats_cover, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->covers->ShowStatistics(GTK_WINDOW(data));
                   }),
                   parent);

  GtkWidget *fetch_lyrics = gtk_button_new_with_label("Fetch lyrics");
  g_object_set_data(G_OBJECT(fetch_lyrics), "state", state);
  g_signal_connect(fetch_lyrics, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     application->lyrics_providers()->Fetch(state->song, [button, state](const std::string &lyrics, const std::string &) {
                       if (lyrics.empty()) {
                         gtk_button_set_label(button, "Missing");
                         return;
                       }
                       state->song.set_lyrics(lyrics);
                       if (state->lyrics) {
                         gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), lyrics.c_str(), -1);
                       }
                       gtk_button_set_label(button, "Fetched");
                     });
                   })),
                   app);

  gtk_box_append(GTK_BOX(box), switcher);
  gtk_widget_set_vexpand(GTK_WIDGET(stack), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(stack));
  gtk_box_append(GTK_BOX(box), save);
  gtk_box_append(GTK_BOX(box), fetch_lyrics);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
