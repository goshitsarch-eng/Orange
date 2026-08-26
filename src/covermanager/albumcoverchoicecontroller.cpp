#include "covermanager/albumcoverchoicecontroller.h"

#include "core/application.h"
#include "covermanager/albumcoversearcher.h"
#include "covermanager/coverproviders.h"
#include "covermanager/coversearchstatisticsdialog.h"
#include "dialogs/dialoghelpers.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <adwaita.h>

#include <algorithm>

AlbumCoverChoiceController::AlbumCoverChoiceController(Application *app) : app_(app) {}

bool AlbumCoverChoiceController::IsKnownImageExtension(const std::string &extension) {
  const std::string ext = StrUtils::ToLower(extension);
  const std::vector<std::string> known = ImageExtensions();
  return std::find(known.begin(), known.end(), ext) != known.end();
}

std::vector<std::string> AlbumCoverChoiceController::ImageExtensions() {
  return {"jpg", "jpeg", "png", "gif", "bmp", "webp"};
}

bool AlbumCoverChoiceController::SaveCover(Application *app, Song *song, const std::string &image) {
  return DialogHelpers::ApplyCover(app, song, image);
}

void AlbumCoverChoiceController::ApplyImage(Song *song, GtkWidget *image, const std::string &data) {
  if (image && !data.empty()) {
    DialogHelpers::SetImageFromBytes(image, std::vector<unsigned char>(data.begin(), data.end()), 160);
  } else if (image) {
    DialogHelpers::SetImageFromBytes(image, {}, 160);
  }
  (void)song;
}

void AlbumCoverChoiceController::FetchCover(Song *song, GtkWidget *image, GtkWidget *status) {
  if (!app_ || !song) {
    return;
  }
  ++statistics_.network_requests_made;
  app_->cover_providers()->Fetch(*song, [this, song, image, status](const std::string &data, const std::string &) {
    if (data.empty()) {
      ++statistics_.missing_images;
      if (status) gtk_button_set_label(GTK_BUTTON(status), "Failed");
      return;
    }
    statistics_.bytes_transferred += data.size();
    if (SaveCover(app_, song, data)) {
      ++statistics_.chosen_images;
      ApplyImage(song, image, data);
      if (status) gtk_button_set_label(GTK_BUTTON(status), "Saved");
    } else if (status) {
      gtk_button_set_label(GTK_BUTTON(status), "Failed");
    }
  });
}

void AlbumCoverChoiceController::SearchForCover(GtkWindow *parent) {
  if (app_) {
    AlbumCoverSearcher::Show(parent, app_);
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
                         if (SaveCover(self->app_, song, result.body)) {
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
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Images");
  for (const std::string &ext : ImageExtensions()) {
    gtk_file_filter_add_suffix(filter, ext.c_str());
  }
  GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
  g_list_store_append(filters, filter);
  gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
  g_object_unref(filters);
  g_object_unref(filter);
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
        if (!bytes.empty() && SaveCover(pair->controller->app_, pair->song, bytes)) {
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
