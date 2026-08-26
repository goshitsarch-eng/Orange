#include "smartplaylists/smartplaylistwizard.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"
#include "playlist/playlist.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylistsearchpreview.h"
#include "smartplaylists/smartplaylistsearchtermwidget.h"
#include "smartplaylists/smartplaylistwizardfinishpage.h"
#include "smartplaylists/smartplaylistwizardplugin.h"
#include "smartplaylists/smartplaylistwizardtypepage.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using DialogHelpers::DropDownFromNames;

void SmartPlaylistWizard::Show(GtkWindow *parent, Application *app) { Show(parent, app, {}, {}); }

void SmartPlaylistWizard::Show(GtkWindow *parent, Application *app, const std::string &name, const SmartPlaylistSearch &search) {
  AdwDialog *dialog = adw_dialog_new();
  const bool editing = !name.empty();
  adw_dialog_set_title(dialog, editing ? Translations::CStr("Edit smart playlist") : Translations::CStr("Smart playlist"));
  adw_dialog_set_content_width(dialog, 540);
  adw_dialog_set_content_height(dialog, 760);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  struct WizardState {
    std::unique_ptr<SmartPlaylistWizardTypePage> type;
    GtkWidget *match = nullptr;
    GtkWidget *terms_box = nullptr;
    std::vector<std::unique_ptr<SmartPlaylistSearchTermWidget>> terms;
    GtkWidget *limit = nullptr;
    GtkWidget *sort = nullptr;
    GtkWidget *descending = nullptr;
    std::unique_ptr<SmartPlaylistSearchPreview> preview;
    std::unique_ptr<SmartPlaylistWizardFinishPage> finish;
    std::string original_name;
    bool editing = false;

    SmartPlaylistSearch Build() const {
      SmartPlaylistSearch result;
      result.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(match)) == 1 ? SmartPlaylistSearch::SearchType::Or
                                                                         : SmartPlaylistSearch::SearchType::And;
      for (const auto &term : terms) {
        if (!term->IsEmpty()) {
          result.terms.push_back(term->Term());
        }
      }
      result.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(limit)));
      result.sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(sort))));
      result.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(descending));
      return result;
    }

    void AddTerm(const SmartPlaylistTerm *term = nullptr) {
      auto widget = std::make_unique<SmartPlaylistSearchTermWidget>();
      if (term) {
        widget->SetTerm(*term);
      }
      gtk_box_append(GTK_BOX(terms_box), widget->widget());
      terms.push_back(std::move(widget));
    }

    void RestoreDefaults() {
      gtk_drop_down_set_selected(GTK_DROP_DOWN(match), 0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit), 100);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(sort), 0);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(descending), FALSE);
      for (auto &term : terms) {
        term->SetTerm({});
      }
    }
  };
  auto *state = new WizardState();
  state->type = std::make_unique<SmartPlaylistWizardTypePage>();
  state->match = DropDownFromNames({"Match all terms (AND)", "Match any term (OR)"});
  state->preview = std::make_unique<SmartPlaylistSearchPreview>();
  state->finish = std::make_unique<SmartPlaylistWizardFinishPage>();
  state->limit = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit), 100);
  state->sort = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  state->descending = gtk_check_button_new_with_label(Translations::CStr("Sort descending"));
  state->terms_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  state->original_name = name;
  state->editing = editing;

  gtk_box_append(GTK_BOX(box), state->type->widget());
  gtk_box_append(GTK_BOX(box), state->match);
  gtk_box_append(GTK_BOX(box), state->terms_box);
  const int initial_terms = editing ? std::max(3, static_cast<int>(search.terms.size())) : 3;
  for (int i = 0; i < initial_terms; ++i) {
    if (editing && i < static_cast<int>(search.terms.size())) {
      state->AddTerm(&search.terms[static_cast<size_t>(i)]);
    } else {
      state->AddTerm();
    }
  }
  GtkWidget *add_term = gtk_button_new_with_label(Translations::CStr("Add term"));
  g_object_set_data(G_OBJECT(add_term), "wizard", state);
  g_signal_connect(add_term, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     wizard->AddTerm();
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), add_term);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Limit (0 = no limit)")));
  gtk_box_append(GTK_BOX(box), state->limit);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Sort by")));
  gtk_box_append(GTK_BOX(box), state->sort);
  gtk_box_append(GTK_BOX(box), state->descending);
  gtk_box_append(GTK_BOX(box), state->preview->widget());
  gtk_box_append(GTK_BOX(box), state->finish->widget());

  if (editing) {
    state->type->SetName(name);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->match), search.type == SmartPlaylistSearch::SearchType::Or ? 1 : 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit), search.limit);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->sort), static_cast<guint>(search.sort_field));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->descending), search.sort_descending);
  }

  GtkWidget *preview = gtk_button_new_with_label(Translations::CStr("Preview"));
  GtkWidget *restore = gtk_button_new_with_label(Translations::CStr("Restore defaults"));
  GtkWidget *create = gtk_button_new_with_label(editing ? Translations::CStr("Save") : Translations::CStr("Create"));
  gtk_widget_add_css_class(create, "suggested-action");
  g_object_set_data_full(G_OBJECT(create), "wizard", state, [](gpointer p) { delete static_cast<WizardState *>(p); });
  g_object_set_data(G_OBJECT(preview), "wizard", state);
  g_object_set_data(G_OBJECT(restore), "wizard", state);
  g_signal_connect(preview, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SmartPlaylistSearch current = wizard->Build();
                     wizard->preview->Update(current, application->collection()->Songs());
                     wizard->finish->SetSummary(std::to_string(wizard->preview->match_count()) + " songs will be added as “" +
                                                wizard->type->name() + "”.");
                   })),
                   app);
  g_signal_connect(restore, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     wizard->RestoreDefaults();
                   }),
                   nullptr);
  g_signal_connect(create, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SmartPlaylistSearch current = wizard->Build();
                     const std::string saved_name = wizard->type->name();
                     if (wizard->editing) {
                       SmartPlaylistSearch::RenameSaved(wizard->original_name, saved_name, current);
                       return;
                     }
                     SmartPlaylistQueryWizardPlugin plugin(current);
                     auto generator = plugin.CreateGenerator(saved_name, wizard->type->dynamic());
                     generator->set_collection_backend(application->collection()->backend());
                     Playlist *playlist = application->playlist_manager()->New(saved_name);
                     SmartPlaylistSearch::AddSaved(saved_name, current);
                     if (wizard->type->dynamic()) {
                       playlist->SetDynamic(true, current);
                     }
                     PlaylistGeneratorInserter inserter;
                     inserter.Insert(playlist, generator);
                   })),
                   app);
  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(actions), restore);
  gtk_box_append(GTK_BOX(actions), preview);
  gtk_box_append(GTK_BOX(actions), create);
  gtk_box_append(GTK_BOX(box), actions);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
