#include "smartplaylists/smartplaylistwizard.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"
#include "playlist/playlist.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylistsearchpreview.h"
#include "smartplaylists/smartplaylistsearchtermwidget.h"
#include "smartplaylists/smartplaylistsummary.h"
#include "smartplaylists/smartplaylistwizardfinishpage.h"
#include "smartplaylists/smartplaylistwizardlabels.h"
#include "smartplaylists/smartplaylistwizardplugin.h"
#include "smartplaylists/smartplaylistwizardtypepage.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using DialogHelpers::DropDownFromNames;
namespace Labels = SmartPlaylistWizardLabels;

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
    GtkWidget *add_term = nullptr;
    std::vector<std::unique_ptr<SmartPlaylistSearchTermWidget>> terms;
    GtkWidget *random = nullptr;
    GtkWidget *field_sort = nullptr;
    GtkWidget *limit_none = nullptr;
    GtkWidget *limit_limit = nullptr;
    GtkWidget *limit = nullptr;
    GtkWidget *sort = nullptr;
    GtkWidget *descending = nullptr;
    std::unique_ptr<SmartPlaylistSearchPreview> preview;
    std::unique_ptr<SmartPlaylistWizardFinishPage> finish;
    std::string original_name;
    bool editing = false;

    SmartPlaylistSearch Build() const {
      SmartPlaylistSearch result;
      result.type = Labels::TypeFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(match))));
      for (const auto &term : terms) {
        if (!term->IsEmpty()) {
          result.terms.push_back(term->Term());
        }
      }
      result.limit = Labels::LimitFromUi(gtk_check_button_get_active(GTK_CHECK_BUTTON(limit_none)),
                                         static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(limit))));
      result.sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(sort))));
      result.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(descending));
      result.sort_random = gtk_check_button_get_active(GTK_CHECK_BUTTON(random));
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

    void ApplyTermsSensitive() {
      const bool on = Labels::TermsSensitive(Labels::TypeFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(match)))));
      gtk_widget_set_sensitive(terms_box, on);
      if (add_term) {
        gtk_widget_set_sensitive(add_term, on);
      }
    }

    void ApplySortSensitive() {
      const bool field = Labels::FieldSortSensitive(gtk_check_button_get_active(GTK_CHECK_BUTTON(random)));
      gtk_widget_set_sensitive(sort, field);
      gtk_widget_set_sensitive(descending, field);
    }

    void ApplyLimitSensitive() {
      gtk_widget_set_sensitive(limit, Labels::LimitSpinSensitive(gtk_check_button_get_active(GTK_CHECK_BUTTON(limit_none))));
    }

    void RestoreDefaults() {
      gtk_drop_down_set_selected(GTK_DROP_DOWN(match), 0);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(random), TRUE);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit), 15);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(limit_none), TRUE);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(sort), 0);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(descending), FALSE);
      for (auto &term : terms) {
        term->SetTerm({});
      }
      ApplyTermsSensitive();
      ApplySortSensitive();
      ApplyLimitSensitive();
    }
  };
  auto *state = new WizardState();
  state->type = std::make_unique<SmartPlaylistWizardTypePage>();
  state->match = DropDownFromNames({Labels::And(), Labels::Or(), Labels::All()});
  state->preview = std::make_unique<SmartPlaylistSearchPreview>();
  state->finish = std::make_unique<SmartPlaylistWizardFinishPage>();
  state->limit = gtk_spin_button_new_with_range(1, 1000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit), 15);
  state->sort = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  state->descending = gtk_check_button_new_with_label(Translations::CStr("Sort descending"));
  state->terms_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  state->random = gtk_check_button_new_with_label(Translations::CStr(Labels::Random()));
  state->field_sort = gtk_check_button_new_with_label(Translations::CStr(Labels::SortBy()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(state->field_sort), GTK_CHECK_BUTTON(state->random));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->random), TRUE);
  state->limit_none = gtk_check_button_new_with_label(Translations::CStr(Labels::ShowAll()));
  state->limit_limit = gtk_check_button_new_with_label(Translations::CStr(Labels::OnlyFirst()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(state->limit_limit), GTK_CHECK_BUTTON(state->limit_none));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->limit_none), TRUE);
  state->original_name = name;
  state->editing = editing;

  gtk_box_append(GTK_BOX(box), state->type->widget());
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(Labels::SearchMode())));
  gtk_box_append(GTK_BOX(box), state->match);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(Labels::SearchTerms())));
  gtk_box_append(GTK_BOX(box), state->terms_box);
  const int initial_terms = editing ? std::max(3, static_cast<int>(search.terms.size())) : 3;
  for (int i = 0; i < initial_terms; ++i) {
    if (editing && i < static_cast<int>(search.terms.size())) {
      state->AddTerm(&search.terms[static_cast<size_t>(i)]);
    } else {
      state->AddTerm();
    }
  }
  state->add_term = gtk_button_new_with_label(Translations::CStr("Add term"));
  g_object_set_data(G_OBJECT(state->add_term), "wizard", state);
  g_signal_connect(state->add_term, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     wizard->AddTerm();
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), state->add_term);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(Labels::Sorting())));
  gtk_box_append(GTK_BOX(box), state->random);
  gtk_box_append(GTK_BOX(box), state->field_sort);
  gtk_box_append(GTK_BOX(box), state->sort);
  gtk_box_append(GTK_BOX(box), state->descending);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(Labels::Limits())));
  gtk_box_append(GTK_BOX(box), state->limit_none);
  gtk_box_append(GTK_BOX(box), state->limit_limit);
  GtkWidget *limit_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(limit_row), state->limit);
  gtk_box_append(GTK_BOX(limit_row), gtk_label_new(Translations::CStr(Labels::Songs())));
  gtk_box_append(GTK_BOX(box), limit_row);
  gtk_box_append(GTK_BOX(box), state->preview->widget());
  gtk_box_append(GTK_BOX(box), state->finish->widget());

  if (editing) {
    state->type->SetName(name);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->match), static_cast<guint>(Labels::TypeIndex(search.type)));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit), Labels::LimitSpinOrDefault(search.limit));
    if (search.limit > 0) {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(state->limit_limit), TRUE);
    } else {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(state->limit_none), TRUE);
    }
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->sort), static_cast<guint>(search.sort_field));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->descending), search.sort_descending);
    if (search.sort_random) {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(state->random), TRUE);
    } else {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(state->field_sort), TRUE);
    }
  }
  state->ApplyTermsSensitive();
  state->ApplySortSensitive();
  state->ApplyLimitSensitive();

  g_signal_connect(state->match, "notify::selected", G_CALLBACK(+[](GtkDropDown *, GParamSpec *, gpointer data) {
                     static_cast<WizardState *>(data)->ApplyTermsSensitive();
                   }),
                   state);
  g_signal_connect(state->random, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) {
                     static_cast<WizardState *>(data)->ApplySortSensitive();
                   }),
                   state);
  g_signal_connect(state->limit_none, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) {
                     static_cast<WizardState *>(data)->ApplyLimitSensitive();
                   }),
                   state);

  GtkWidget *preview = gtk_button_new_with_label(Translations::CStr("Preview"));
  GtkWidget *restore = gtk_button_new_with_label(Translations::CStr("Restore defaults"));
  GtkWidget *create = gtk_button_new_with_label(editing ? Translations::CStr("Save") : Translations::CStr("Create"));
  gtk_widget_add_css_class(create, "suggested-action");
  gtk_widget_set_sensitive(create, SmartPlaylistWizardFinishPage::IsComplete(state->type->name()));
  g_signal_connect(state->type->name_widget(), "changed", G_CALLBACK(+[](GtkEditable *editable, gpointer data) {
                     const char *text = gtk_editable_get_text(editable);
                     gtk_widget_set_sensitive(GTK_WIDGET(data), SmartPlaylistWizardFinishPage::IsComplete(text ? text : ""));
                   }),
                   create);
  g_object_set_data_full(G_OBJECT(create), "wizard", state, [](gpointer p) { delete static_cast<WizardState *>(p); });
  g_object_set_data(G_OBJECT(preview), "wizard", state);
  g_object_set_data(G_OBJECT(restore), "wizard", state);
  g_signal_connect(preview, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<WizardState *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SmartPlaylistSearch current = wizard->Build();
                     wizard->preview->Update(current, application->collection()->Songs());
                     wizard->finish->SetSummary(SmartPlaylistSummary::FinishText(wizard->preview->match_count(), wizard->type->name(), current));
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
                     if (!SmartPlaylistWizardFinishPage::IsComplete(saved_name)) {
                       return;
                     }
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
