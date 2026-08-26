#include "covermanager/albumcovermanager.h"

#include "core/application.h"
#include "covermanager/albumcoverexportdialog.h"
#include "covermanager/coverfromurldialog.h"
#include "covermanager/coverproviders.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"

#include <adwaita.h>

using DialogHelpers::ApplyCover;
using DialogHelpers::SetImageFromBytes;

void AlbumCoverManager::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Cover manager"));
  adw_dialog_set_content_width(dialog, 640);
  adw_dialog_set_content_height(dialog, 640);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *status = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  gtk_box_append(GTK_BOX(box), status);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *flow = gtk_flow_box_new();
  gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow), 2);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 4);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_NONE);
  std::string last;
  int albums = 0;
  int with_cover = 0;
  auto *missing = new std::vector<GtkWidget *>();
  g_object_set_data_full(G_OBJECT(dialog), "missing", missing, [](gpointer p) { delete static_cast<std::vector<GtkWidget *> *>(p); });
  for (const Song &song : app->collection()->Songs()) {
    const std::string album = song.EffectiveAlbumartist() + " – " + song.album();
    if (album == last || song.album().empty()) {
      continue;
    }
    last = album;
    ++albums;
    auto *copy = new Song(song);
    const auto cover = app->albumcover_loader()->LoadData(song);
    if (!cover.empty()) {
      ++with_cover;
    }
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(card, 140, -1);
    GtkWidget *image = gtk_image_new();
    SetImageFromBytes(image, cover, 120);
    GtkWidget *title = gtk_label_new(song.album().c_str());
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    GtkWidget *artist = gtk_label_new(song.EffectiveAlbumartist().c_str());
    gtk_widget_add_css_class(artist, "dim-label");
    gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_END);
    GtkWidget *button = gtk_button_new_with_label(cover.empty() ? "Fetch" : "Replace");
    gtk_widget_add_css_class(button, "flat");
    g_object_set_data_full(G_OBJECT(button), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    g_object_set_data(G_OBJECT(button), "image", image);
    g_signal_connect(button, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"));
                       GtkWidget *image = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "image"));
                       if (!song) {
                         return;
                       }
                       application->cover_providers()->Fetch(*song, [btn, application, song, image](const std::string &image_data, const std::string &) {
                         if (ApplyCover(application, song, image_data)) {
                           SetImageFromBytes(image, std::vector<unsigned char>(image_data.begin(), image_data.end()), 120);
                           gtk_button_set_label(GTK_BUTTON(btn), "Saved");
                           return;
                         }
                         gtk_button_set_label(GTK_BUTTON(btn), "Failed");
                       });
                     })),
                     app);
    gtk_box_append(GTK_BOX(card), image);
    gtk_box_append(GTK_BOX(card), title);
    gtk_box_append(GTK_BOX(card), artist);
    gtk_box_append(GTK_BOX(card), button);
    gtk_flow_box_append(GTK_FLOW_BOX(flow), card);
    if (cover.empty()) {
      missing->push_back(button);
    }
  }
  gtk_label_set_text(GTK_LABEL(status),
                     (std::to_string(albums) + " albums · " + std::to_string(with_cover) + " with artwork · " +
                      std::to_string(albums - with_cover) + " missing")
                         .c_str());
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), flow);
  gtk_box_append(GTK_BOX(box), scroll);
  GtkWidget *batch = gtk_button_new_with_label(Translations::CStr("Fetch all missing"));
  gtk_widget_add_css_class(batch, "suggested-action");
  g_object_set_data(G_OBJECT(batch), "dialog", dialog);
  g_signal_connect(batch, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer) {
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
                     auto *missing = static_cast<std::vector<GtkWidget *> *>(g_object_get_data(G_OBJECT(dlg), "missing"));
                     if (!missing) {
                       return;
                     }
                     for (GtkWidget *button : *missing) {
                       if (GTK_IS_BUTTON(button)) {
                         gtk_widget_activate(button);
                       }
                     }
                     gtk_button_set_label(btn, "Fetching…");
                   }),
                   nullptr);
  GtkWidget *current = gtk_button_new_with_label(Translations::CStr("Fetch cover for current song"));
  g_signal_connect(current, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     Song song = application->player()->current_song();
                     application->cover_providers()->Fetch(song, [application, song, btn](const std::string &image, const std::string &) {
                       if (!image.empty() && CoverProviders::SaveAlbumCover(song, image, application->tagreader())) {
                         gtk_button_set_label(btn, "Saved");
                       }
                     });
                   })),
                   app);
  GtkWidget *from_url = gtk_button_new_with_label(Translations::CStr("Load cover from URL…"));
  g_signal_connect(from_url, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     CoverFromUrlDialog::Show(nullptr, application);
                   }),
                   app);
  GtkWidget *export_btn = gtk_button_new_with_label(Translations::CStr("Export covers…"));
  g_signal_connect(export_btn, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     AlbumCoverExportDialog::Show(nullptr, application);
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), batch);
  gtk_box_append(GTK_BOX(box), current);
  gtk_box_append(GTK_BOX(box), from_url);
  gtk_box_append(GTK_BOX(box), export_btn);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
