#include "smartplaylists/smartplaylistwizard.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylistsearchpreview.h"
#include "smartplaylists/smartplaylistsearchtermwidget.h"
#include "smartplaylists/smartplaylistwizardfinishpage.h"
#include "smartplaylists/smartplaylistwizardplugin.h"
#include "smartplaylists/smartplaylistwizardtypepage.h"

#include <adwaita.h>

#include <memory>
#include <vector>

using DialogHelpers::DropDownFromNames;

void SmartPlaylistWizard::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Smart playlist");
  adw_dialog_set_content_width(dialog, 540);
  adw_dialog_set_content_height(dialog, 720);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  struct WizardState {
    std::unique_ptr<SmartPlaylistWizardTypePage> type;
    GtkWidget *match = nullptr;
    std::vector<std::unique_ptr<SmartPlaylistSearchTermWidget>> terms;
    GtkWidget *limit = nullptr;
    GtkWidget *sort = nullptr;
    GtkWidget *descending = nullptr;
    std::unique_ptr<SmartPlaylistSearchPreview> preview;
    std::unique_ptr<SmartPlaylistWizardFinishPage> finish;
  };
  auto *state = new WizardState();
  state->type = std::make_unique<SmartPlaylistWizardTypePage>();
  state->match = DropDownFromNames({"Match all terms (AND)", "Match any term (OR)"});
  state->preview = std::make_unique<SmartPlaylistSearchPreview>();
  state->finish = std::make_unique<SmartPlaylistWizardFinishPage>();
  state->limit = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit), 100);
  state->sort = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  state->descending = gtk_check_button_new_with_label("Sort descending");

  gtk_box_append(GTK_BOX(box), state->type->widget());
  gtk_box_append(GTK_BOX(box), state->match);
  for (int i = 0; i < 3; ++i) {
    auto term = std::make_unique<SmartPlaylistSearchTermWidget>();
    gtk_box_append(GTK_BOX(box), term->widget());
    state->terms.push_back(std::move(term));
  }
  gtk_box_append(GTK_BOX(box), gtk_label_new("Limit (0 = no limit)"));
  gtk_box_append(GTK_BOX(box), state->limit);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Sort by"));
  gtk_box_append(GTK_BOX(box), state->sort);
  gtk_box_append(GTK_BOX(box), state->descending);
  gtk_box_append(GTK_BOX(box), state->preview->widget());
  gtk_box_append(GTK_BOX(box), state->finish->widget());

  GtkWidget *preview = gtk_button_new_with_label("Preview");
  GtkWidget *create = gtk_button_new_with_label("Create");
  gtk_widget_add_css_class(create, "suggested-action");
  g_object_set_data_full(G_OBJECT(create), "wizard", state, [](gpointer p) { delete static_cast<WizardState *>(p); });
  g_object_set_data(G_OBJECT(preview), "wizard", state);
  g_signal_connect(preview, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SmartPlaylistSearch search = [&]() {
                       SmartPlaylistSearch result;
                       result.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->match)) == 1
                                         ? SmartPlaylistSearch::SearchType::Or
                                         : SmartPlaylistSearch::SearchType::And;
                       for (const auto &term : wizard->terms) {
                         if (!term->IsEmpty()) result.terms.push_back(term->Term());
                       }
                       result.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(wizard->limit)));
                       result.sort_field =
                           SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->sort))));
                       result.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->descending));
                       return result;
                     }();
                     wizard->preview->Update(search, application->collection()->Songs());
                     wizard->finish->SetSummary(std::to_string(wizard->preview->match_count()) + " songs will be added as “" +
                                                wizard->type->name() + "”.");
                   })),
                   app);
  g_signal_connect(create, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     SmartPlaylistSearch search;
                     search.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->match)) == 1 ? SmartPlaylistSearch::SearchType::Or
                                                                                                : SmartPlaylistSearch::SearchType::And;
                     for (const auto &term : wizard->terms) {
                       if (!term->IsEmpty()) search.terms.push_back(term->Term());
                     }
                     search.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(wizard->limit)));
                     search.sort_field =
                         SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->sort))));
                     search.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->descending));
                     const std::string saved_name = wizard->type->name();
                     SmartPlaylistQueryWizardPlugin plugin(search);
                     auto generator = plugin.CreateGenerator(saved_name, wizard->type->dynamic());
                     generator->set_collection_backend(application->collection()->backend());
                     Playlist *playlist = application->playlist_manager()->New(saved_name);
                     SmartPlaylistSearch::AddSaved(saved_name, search);
                     if (wizard->type->dynamic()) {
                       playlist->SetDynamic(true, search);
                     }
                     PlaylistGeneratorInserter inserter;
                     inserter.Insert(playlist, generator);
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), preview);
  gtk_box_append(GTK_BOX(box), create);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
