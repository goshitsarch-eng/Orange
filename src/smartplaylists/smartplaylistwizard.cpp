#include "smartplaylists/smartplaylistwizard.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"
#include "smartplaylists/smartplaylist.h"

#include <adwaita.h>
#include <algorithm>

using DialogHelpers::DropDownFromNames;

namespace {

struct SmartTermRow {
  GtkWidget *field = nullptr;
  GtkWidget *op = nullptr;
  GtkWidget *value = nullptr;
};

struct SmartWizard {
  GtkWidget *name = nullptr;
  GtkWidget *match = nullptr;
  SmartTermRow terms[3];
  GtkWidget *limit = nullptr;
  GtkWidget *sort = nullptr;
  GtkWidget *descending = nullptr;
  GtkWidget *dynamic = nullptr;
  GtkWidget *preview = nullptr;
};

SmartPlaylistSearch SearchFromWizard(SmartWizard *wizard) {
  SmartPlaylistSearch search;
  search.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->match)) == 1 ? SmartPlaylistSearch::SearchType::Or
                                                                             : SmartPlaylistSearch::SearchType::And;
  for (const SmartTermRow &term : wizard->terms) {
    const int field_i = static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(term.field)));
    const int op_i = static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(term.op)));
    const SmartPlaylistOp op = SmartPlaylistSearch::OpFromIndex(op_i);
    const std::string value = gtk_editable_get_text(GTK_EDITABLE(term.value));
    if (value.empty() && op != SmartPlaylistOp::Empty && op != SmartPlaylistOp::NotEmpty) {
      continue;
    }
    search.terms.push_back({SmartPlaylistSearch::FieldFromIndex(field_i), op, value});
  }
  search.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(wizard->limit)));
  search.sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->sort))));
  search.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->descending));
  return search;
}

}  // namespace

void SmartPlaylistWizard::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Smart playlist");
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 640);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Playlist name");
  GtkWidget *match = DropDownFromNames({"Match all terms (AND)", "Match any term (OR)"});
  auto *wizard = new SmartWizard();
  wizard->name = name;
  wizard->match = match;
  GtkWidget *terms_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  for (int i = 0; i < 3; ++i) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    wizard->terms[i].field = DropDownFromNames(SmartPlaylistSearch::FieldNames());
    wizard->terms[i].op = DropDownFromNames(SmartPlaylistSearch::OpNames());
    wizard->terms[i].value = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(wizard->terms[i].value), i == 0 ? "Value" : "Optional term");
    gtk_widget_set_hexpand(wizard->terms[i].value, TRUE);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].field);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].op);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].value);
    gtk_box_append(GTK_BOX(terms_box), row);
  }
  wizard->limit = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(wizard->limit), 100);
  wizard->sort = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  wizard->descending = gtk_check_button_new_with_label("Sort descending");
  wizard->dynamic = gtk_check_button_new_with_label("Dynamic (keep refilling as tracks play)");
  wizard->preview = gtk_label_new("Preview shows matching tracks from the collection.");
  gtk_label_set_wrap(GTK_LABEL(wizard->preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(wizard->preview), 0);
  GtkWidget *preview = gtk_button_new_with_label("Preview");
  GtkWidget *create = gtk_button_new_with_label("Create");
  gtk_widget_add_css_class(create, "suggested-action");
  g_object_set_data_full(G_OBJECT(create), "wizard", wizard, [](gpointer p) { delete static_cast<SmartWizard *>(p); });
  g_object_set_data(G_OBJECT(preview), "wizard", wizard);
  g_signal_connect(preview, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<SmartWizard *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SongList songs = SearchFromWizard(wizard).Search(application->collection()->Songs());
                     std::string text = std::to_string(songs.size()) + " matches";
                     for (size_t i = 0; i < songs.size() && i < 8; ++i) {
                       text += "\n" + songs[i].PrettyTitleWithArtist();
                     }
                     gtk_label_set_text(GTK_LABEL(wizard->preview), text.c_str());
                   })),
                   app);
  g_signal_connect(create, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<SmartWizard *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     SmartPlaylistSearch search = SearchFromWizard(wizard);
                     const char *playlist_name = gtk_editable_get_text(GTK_EDITABLE(wizard->name));
                     const std::string saved_name = playlist_name && *playlist_name ? playlist_name : "Smart playlist";
                     Playlist *playlist = application->playlist_manager()->New(saved_name);
                     SmartPlaylistSearch::AddSaved(saved_name, search);
                     if (gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->dynamic))) {
                       playlist->SetDynamic(true, search);
                       search.limit = search.limit > 0 ? std::min(search.limit, 20) : 20;
                     }
                     application->playlist_manager()->AppendSongs(search.Search(application->collection()->Songs()));
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), match);
  gtk_box_append(GTK_BOX(box), terms_box);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Limit (0 = no limit)"));
  gtk_box_append(GTK_BOX(box), wizard->limit);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Sort by"));
  gtk_box_append(GTK_BOX(box), wizard->sort);
  gtk_box_append(GTK_BOX(box), wizard->descending);
  gtk_box_append(GTK_BOX(box), wizard->dynamic);
  gtk_box_append(GTK_BOX(box), preview);
  gtk_box_append(GTK_BOX(box), wizard->preview);
  gtk_box_append(GTK_BOX(box), create);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
