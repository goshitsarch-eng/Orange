#include "covermanager/albumcoverchoicecontroller.h"

#include "constants/coverssettings.h"
#include "context/contextcover.h"
#include "core/application.h"
#include "core/settings.h"
#include "covermanager/albumcoversearcher.h"
#include "covermanager/coverproviders.h"
#include "covermanager/coversearchstatisticsdialog.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"
#include "constants/filefilterconstants.h"
#include "utilities/filefilters.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <adwaita.h>

#include <algorithm>
#include <memory>

AlbumCoverChoiceController::AlbumCoverChoiceController(Application *app) : app_(app) { ReloadSettings(); }

void AlbumCoverChoiceController::ReloadSettings() { cover_options_ = CoverOptions::LoadFromSettings(); }

bool AlbumCoverChoiceController::IsKnownImageExtension(const std::string &extension) {
  const std::string ext = StrUtils::ToLower(extension);
  const std::vector<std::string> known = ImageExtensions();
  return std::find(known.begin(), known.end(), ext) != known.end();
}

std::vector<std::string> AlbumCoverChoiceController::ImageExtensions() {
  std::vector<std::string> extensions;
  for (const std::string &glob : FileFilterConstants::SplitGlobs(FileFilterConstants::kLoadImages)) {
    if (glob.size() > 2 && glob[0] == '*' && glob[1] == '.') {
      extensions.push_back(glob.substr(2));
    }
  }
  return extensions;
}

bool AlbumCoverChoiceController::SaveCover(Application *app, Song *song, const std::string &image) {
  return DialogHelpers::ApplyCover(app, song, image);
}

bool AlbumCoverChoiceController::SaveCover(Song *song, const std::string &image) {
  return DialogHelpers::ApplyCover(app_, song, image, EffectiveOptions());
}

void AlbumCoverChoiceController::ApplyImage(Song *song, GtkWidget *image, const std::string &data) {
  if (image && !data.empty()) {
    DialogHelpers::SetImageFromBytes(image, std::vector<unsigned char>(data.begin(), data.end()), 160);
  } else if (image) {
    DialogHelpers::SetImageFromBytes(image, {}, 160);
  }
  (void)song;
}

void AlbumCoverChoiceController::FetchCover(Song *song, GtkWidget *image, GtkWidget *status, std::function<void(bool)> done) {
  if (!app_ || !song) {
    if (done) {
      done(false);
    }
    return;
  }
  ++statistics_.network_requests_made;
  auto owned = std::make_shared<Song>(*song);
  app_->cover_providers()->Fetch(*owned, [this, owned, song, image, status, done](const std::string &data, const std::string &) {
    if (data.empty()) {
      ++statistics_.missing_images;
      if (status) gtk_button_set_label(GTK_BUTTON(status), "Failed");
      if (done) {
        done(false);
      }
      return;
    }
    statistics_.bytes_transferred += data.size();
    if (SaveCover(owned.get(), data)) {
      ++statistics_.chosen_images;
      if (song) {
        *song = *owned;
      }
      ApplyImage(owned.get(), image, data);
      if (status) gtk_button_set_label(GTK_BUTTON(status), "Saved");
      if (done) {
        done(true);
      }
    } else {
      if (status) {
        gtk_button_set_label(GTK_BUTTON(status), "Failed");
      }
      if (done) {
        done(false);
      }
    }
  });
}

void AlbumCoverChoiceController::SearchForCover(GtkWindow *parent) { SearchForCover(parent, Song(), {}); }

void AlbumCoverChoiceController::SearchForCover(GtkWindow *parent, const Song &song, std::function<void(bool)> done) {
  if (app_) {
    AlbumCoverSearcher::Show(parent, app_, song, std::move(done));
  }
}

void AlbumCoverChoiceController::UnsetCover(Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  song->set_art_unset(true);
  song->set_art_manual({});
  song->set_art_automatic({});
  song->set_art_embedded(false);
  if (song->id() > 0) {
    app_->collection()->backend()->AddOrUpdateSong(*song);
  }
  ApplyImage(song, image, {});
}

void AlbumCoverChoiceController::ClearCover(Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  song->set_art_manual({});
  song->set_art_automatic({});
  song->set_art_unset(false);
  if (song->id() > 0) {
    app_->collection()->backend()->AddOrUpdateSong(*song);
  }
  const auto data = app_->albumcover_loader()->LoadData(*song);
  ApplyImage(song, image, std::string(data.begin(), data.end()));
}

void AlbumCoverChoiceController::DeleteCover(Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(song->url());
  if (!path.empty() && FileUtils::Exists(path)) {
    app_->tagreader()->ClearCover(path);
  }
  const std::string dir = FileUtils::DirName(path);
  for (const char *name : {"cover.jpg", "cover.png", "folder.jpg", "front.jpg", "album.jpg"}) {
    FileUtils::Remove(FileUtils::Join(dir, name));
  }
  if (!song->art_manual().empty()) {
    FileUtils::Remove(FileUtils::PathFromUri(song->art_manual()));
  }
  if (!song->art_automatic().empty()) {
    FileUtils::Remove(FileUtils::PathFromUri(song->art_automatic()));
  }
  song->set_art_manual({});
  song->set_art_automatic({});
  song->set_art_embedded(false);
  song->set_art_unset(false);
  if (song->id() > 0) {
    app_->collection()->backend()->AddOrUpdateSong(*song);
  }
  ApplyImage(song, image, {});
}

void AlbumCoverChoiceController::SaveCoverToFile(GtkWindow *parent, const Song &song) {
  if (!app_) {
    return;
  }
  const auto data = app_->albumcover_loader()->LoadData(song);
  if (data.empty()) {
    return;
  }
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Save cover image");
  gtk_file_dialog_set_initial_name(dialog, "cover.jpg");
  FileFilters::Apply(dialog, FileFilters::ImageFilters(true));
  auto *bytes = new std::string(data.begin(), data.end());
  gtk_file_dialog_save(dialog, parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *image = static_cast<std::string *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &error);
    if (file) {
      gchar *path = g_file_get_path(file);
      if (path) {
        FileUtils::WriteFile(path, *image);
        g_free(path);
      }
      g_object_unref(file);
    }
    if (error) {
      g_error_free(error);
    }
    delete image;
    g_object_unref(source);
  }, bytes);
}

void AlbumCoverChoiceController::ShowCover(GtkWindow *parent, const Song &song) {
  if (!app_) {
    return;
  }
  const auto data = app_->albumcover_loader()->LoadData(song);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, song.PrettyTitleWithArtist().c_str());
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 560);
  GtkWidget *image = gtk_image_new();
  gtk_widget_set_hexpand(image, TRUE);
  gtk_widget_set_vexpand(image, TRUE);
  DialogHelpers::SetImageFromBytes(image, data, 480);
  adw_dialog_set_child(dialog, image);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void AlbumCoverChoiceController::SearchCoverAutomatically(Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  Settings settings;
  settings.BeginGroup(CoversSettings::kSettingsGroup);
  const bool covers_automatic = settings.BoolValue(CoversSettings::kAutomaticSearch, CoversSettings::kDefaultAutomaticSearch);
  if (!ContextCover::ShouldSearchForSong(ContextCover::LoadEnabled(settings), covers_automatic, song->art_unset(), song->art_embedded(),
                                         song->art_automatic(), song->art_manual(), song->EffectiveAlbumartist(),
                                         ContextCover::EffectiveAlbum(song->album(), song->title()))) {
    return;
  }
  if (!app_->albumcover_loader()->LoadData(*song).empty()) {
    return;
  }
  FetchCover(song, image, nullptr);
}

void AlbumCoverChoiceController::Perform(CoverChoiceMenu::Action action, GtkWindow *parent, Song *song, GtkWidget *image) {
  switch (action) {
    case CoverChoiceMenu::Action::Show:
      if (song) {
        ShowCover(parent, *song);
      }
      break;
    case CoverChoiceMenu::Action::Search:
      SearchForCover(parent, song ? *song : Song());
      break;
    case CoverChoiceMenu::Action::File:
      LoadCoverFromFile(parent, song, image);
      break;
    case CoverChoiceMenu::Action::Url:
      LoadCoverFromURL(parent, song, image);
      break;
    case CoverChoiceMenu::Action::Save:
      if (song) {
        SaveCoverToFile(parent, *song);
      }
      break;
    case CoverChoiceMenu::Action::Fetch:
      FetchCover(song, image, nullptr);
      break;
    case CoverChoiceMenu::Action::Unset:
      UnsetCover(song, image);
      break;
    case CoverChoiceMenu::Action::Clear:
      ClearCover(song, image);
      break;
    case CoverChoiceMenu::Action::Delete:
      DeleteCover(song, image);
      break;
  }
}

void AlbumCoverChoiceController::AttachMenu(GtkWidget *widget, GtkWindow *parent, const std::function<Song()> &song_for_menu) {
  if (!widget) {
    return;
  }
  auto *holder = new std::function<Song()>(song_for_menu);
  g_object_set_data_full(G_OBJECT(widget), "cover-song-fn", holder, [](gpointer p) { delete static_cast<std::function<Song()> *>(p); });
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
  g_object_set_data(G_OBJECT(gesture), "parent", parent);
  g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble, gpointer data) {
                     auto *self = static_cast<AlbumCoverChoiceController *>(data);
                     GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(click));
                     auto *fn = static_cast<std::function<Song()> *>(g_object_get_data(G_OBJECT(widget), "cover-song-fn"));
                     auto *parent = GTK_WINDOW(g_object_get_data(G_OBJECT(click), "parent"));
                     if (!fn) {
                       return;
                     }
                     Song song = (*fn)();
                     auto *owned = new Song(song);
                     GMenu *menu = g_menu_new();
                     for (const CoverChoiceMenu::Item &item : CoverChoiceMenu::Items()) {
                       g_menu_append(menu, Translations::CStr(item.label), CoverChoiceMenu::ActionPath("cover", item.id).c_str());
                     }
                     GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
                     gtk_widget_set_parent(popover, widget);
                     GSimpleActionGroup *group = g_simple_action_group_new();
                     auto add = [&](const char *name) {
                       GSimpleAction *action = g_simple_action_new(name, nullptr);
                       g_object_set_data(G_OBJECT(action), "song", owned);
                       g_object_set_data(G_OBJECT(action), "parent", parent);
                       g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer controller) {
                                          auto *self = static_cast<AlbumCoverChoiceController *>(controller);
                                          auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(act), "song"));
                                          auto *parent = GTK_WINDOW(g_object_get_data(G_OBJECT(act), "parent"));
                                          const char *name = g_action_get_name(G_ACTION(act));
                                          if (!name) {
                                            return;
                                          }
                                          self->Perform(CoverChoiceMenu::FromId(name), parent, song, nullptr);
                                        }),
                                        self);
                       g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
                     };
                     for (const CoverChoiceMenu::Item &item : CoverChoiceMenu::Items()) {
                       add(item.id);
                     }
                     gtk_widget_insert_action_group(popover, "cover", G_ACTION_GROUP(group));
                     g_object_set_data_full(G_OBJECT(popover), "song", owned, [](gpointer p) { delete static_cast<Song *>(p); });
                     gtk_popover_popup(GTK_POPOVER(popover));
                   }),
                   this);
}

void AlbumCoverChoiceController::LoadCoverFromURL(GtkWindow *parent, Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Cover from URL", "Download artwork for this album."));
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "https://");
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "fetch", "Download", nullptr);
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_object_set_data(G_OBJECT(dialog), "song", song);
  g_object_set_data(G_OBJECT(dialog), "image", image);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "fetch") != 0) {
                       return;
                     }
                     auto *self = static_cast<AlbumCoverChoiceController *>(data);
                     const char *url = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry")));
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(alert), "song"));
                     GtkWidget *image = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "image"));
                     if (!url || !*url || !song) {
                       return;
                     }
                     ++self->statistics_.network_requests_made;
                     self->app_->network()->Get(url, [self, song, image](const NetworkAccessManager::Response &result) {
                       if (result.ok() && JsonUtils::LooksLikeImage(result.body)) {
                         self->statistics_.bytes_transferred += result.body.size();
                         if (self->SaveCover(song, result.body)) {
                           ++self->statistics_.chosen_images;
                           self->ApplyImage(song, image, result.body);
                         }
                       } else {
                         ++self->statistics_.missing_images;
                       }
                     });
                   }),
                   this);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void AlbumCoverChoiceController::LoadCoverFromFile(GtkWindow *parent, Song *song, GtkWidget *image) {
  if (!app_ || !song) {
    return;
  }
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Choose cover image");
  FileFilters::Apply(dialog, FileFilters::ImageFilters(false));
  struct FileState {
    AlbumCoverChoiceController *controller;
    Song *song;
    GtkWidget *image;
  };
  auto *state = new FileState{this, song, image};
  gtk_file_dialog_open(dialog, parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *pair = static_cast<FileState *>(data);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
    if (file) {
      gchar *path = g_file_get_path(file);
      if (path) {
        const std::string bytes = FileUtils::ReadFile(path);
        if (!bytes.empty() && pair->controller->SaveCover(pair->song, bytes)) {
          ++pair->controller->statistics_.chosen_images;
          pair->controller->ApplyImage(pair->song, pair->image, bytes);
        }
        g_free(path);
      }
      g_object_unref(file);
    }
    if (error) {
      g_error_free(error);
    }
    delete pair;
    g_object_unref(source);
  }, state);
}

void AlbumCoverChoiceController::ShowStatistics(GtkWindow *parent) {
  CoverSearchStatisticsDialog::Show(parent, statistics_);
}
