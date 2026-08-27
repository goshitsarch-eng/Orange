#include "radios/radiobrowsersearchview.h"

#include "constants/radiobrowsersettings.h"
#include "core/settings.h"
#include "radios/radiobrowsersearchopts.h"
#include "radios/radiodrag.h"
#include "radios/radiomenu.h"
#include "radios/radioservices.h"
#include "translations/translations.h"

#include <string>

RadioBrowserSearchView::RadioBrowserSearchView(RadioServices *services) : services_(services) {
  Settings settings;
  settings.BeginGroup(RadioBrowserSettings::kSettingsGroup);
  search_limit_ = settings.IntValue(RadioBrowserSettings::kSearchLimit, RadioBrowserSettings::kSearchLimitDefault);
  hide_broken_ = settings.BoolValue(RadioBrowserSettings::kHideBroken, RadioBrowserSettings::kHideBrokenDefault);
  default_country_ = settings.Value(RadioBrowserSettings::kDefaultCountry);
  const std::string default_sort = settings.Value(RadioBrowserSettings::kDefaultSort, RadioBrowserSettings::kDefaultSortDefault);

  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  entry_ = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(entry_), Translations::CStr(RadioBrowserSearchOpts::SearchPlaceholder()));
  gtk_widget_set_margin_start(entry_, 8);
  gtk_widget_set_margin_end(entry_, 8);
  gtk_widget_set_margin_top(entry_, 6);
  gtk_box_append(GTK_BOX(widget_), entry_);

  GtkWidget *filters = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(filters, 8);
  gtk_widget_set_margin_end(filters, 8);
  country_ = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(country_), RadioBrowserSearchOpts::AllCountriesId(),
                            Translations::CStr(RadioBrowserSearchOpts::AllCountriesLabel()));
  gtk_combo_box_set_active(GTK_COMBO_BOX(country_), 0);
  gtk_widget_set_hexpand(country_, TRUE);
  sort_ = gtk_combo_box_text_new();
  for (const RadioBrowserSearchOpts::SortOption &option : RadioBrowserSearchOpts::SortOptions()) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sort_), option.id, Translations::CStr(option.label));
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(sort_), RadioBrowserSearchOpts::SortIndex(default_sort.c_str()));
  gtk_widget_set_hexpand(sort_, TRUE);
  gtk_box_append(GTK_BOX(filters), country_);
  gtk_box_append(GTK_BOX(filters), sort_);
  gtk_box_append(GTK_BOX(widget_), filters);

  status_ = gtk_label_new("");
  gtk_widget_add_css_class(status_, "dim-label");
  gtk_label_set_xalign(GTK_LABEL(status_), 0.0f);
  gtk_widget_set_margin_start(status_, 8);
  gtk_widget_set_margin_end(status_, 8);
  gtk_box_append(GTK_BOX(widget_), status_);

  help_ = gtk_label_new(Translations::CStr(RadioBrowserSearchOpts::HelpText()));
  gtk_widget_add_css_class(help_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(help_), TRUE);
  gtk_label_set_xalign(GTK_LABEL(help_), 0.0f);
  gtk_widget_set_margin_start(help_, 8);
  gtk_widget_set_margin_end(help_, 8);
  gtk_box_append(GTK_BOX(widget_), help_);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  list_ = gtk_list_box_new();
  gtk_widget_add_css_class(list_, "boxed-list");
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_), GTK_SELECTION_MULTIPLE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list_);
  gtk_box_append(GTK_BOX(widget_), scroll);

  load_more_ = gtk_button_new_with_label(Translations::CStr(RadioBrowserSearchOpts::LoadMoreLabel()));
  gtk_widget_set_margin_start(load_more_, 8);
  gtk_widget_set_margin_end(load_more_, 8);
  gtk_widget_set_margin_bottom(load_more_, 6);
  gtk_widget_set_visible(load_more_, FALSE);
  gtk_box_append(GTK_BOX(widget_), load_more_);

  g_signal_connect(entry_, "search-changed", G_CALLBACK(+[](GtkSearchEntry *, gpointer data) {
                     static_cast<RadioBrowserSearchView *>(data)->ScheduleSearch();
                   }),
                   this);
  g_signal_connect(country_, "changed", G_CALLBACK(+[](GtkComboBox *, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     if (self->services_ && !self->applying_countries_) {
                       self->SearchTriggered();
                     }
                   }),
                   this);
  g_signal_connect(sort_, "changed", G_CALLBACK(+[](GtkComboBox *, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     if (self->services_) {
                       self->SearchTriggered();
                     }
                   }),
                   this);
  g_signal_connect(load_more_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<RadioBrowserSearchView *>(data)->LoadMore(); }),
                   this);
  g_signal_connect(list_, "row-activated", G_CALLBACK(+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     auto *channel = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(row), "channel"));
                     if (channel && self->activate_) {
                       self->activate_(*channel);
                     }
                   }),
                   this);
  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(list_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK(+[](GtkGestureClick *click, gint, gdouble, gdouble y, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     if (GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(self->list_), static_cast<int>(y))) {
                       if (!gtk_list_box_row_is_selected(row)) {
                         gtk_list_box_unselect_all(GTK_LIST_BOX(self->list_));
                         gtk_list_box_select_row(GTK_LIST_BOX(self->list_), row);
                       }
                     }
                     if (self->menu_) {
                       self->menu_(self->SelectedChannels());
                     }
                     gtk_gesture_set_state(GTK_GESTURE(click), GTK_EVENT_SEQUENCE_CLAIMED);
                   }),
                   this);
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_widget_add_controller(list_, keys);
  gtk_widget_set_focusable(list_, TRUE);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     return static_cast<RadioBrowserSearchView *>(data)->OnKeyPressed(keyval, state);
                   })),
                   this);

  g_signal_connect(widget_, "map", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     if (!self->countries_loaded_) {
                       self->FetchCountries();
                     }
                   }),
                   this);
}

gboolean RadioBrowserSearchView::OnKeyPressed(guint keyval, GdkModifierType state) {
  if (!RadioMenu::IsKeyboardTrigger(keyval, static_cast<unsigned>(state))) {
    return FALSE;
  }
  if (menu_ && RadioMenu::ShouldShowMenu()) {
    menu_(SelectedChannels());
  }
  return TRUE;
}

RadioBrowserSearchView::~RadioBrowserSearchView() {
  ++search_generation_;
  if (search_timeout_) {
    g_source_remove(search_timeout_);
    search_timeout_ = 0;
  }
}

void RadioBrowserSearchView::SetResults(const std::vector<RadioChannel> &results) {
  model_.SetResults(results);
  ReloadResults();
}

void RadioBrowserSearchView::ScheduleSearch() {
  if (changed_) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry_));
    changed_(text ? text : "");
  }
  if (search_timeout_) {
    g_source_remove(search_timeout_);
  }
  search_timeout_ = g_timeout_add(RadioBrowserSearchOpts::DebounceMs(), [](gpointer data) -> gboolean {
    auto *self = static_cast<RadioBrowserSearchView *>(data);
    self->search_timeout_ = 0;
    self->SearchTriggered();
    return G_SOURCE_REMOVE;
  }, this);
}

void RadioBrowserSearchView::SearchTriggered() {
  current_offset_ = 0;
  model_.Clear();
  DoSearch();
}

void RadioBrowserSearchView::LoadMore() {
  current_offset_ = RadioBrowserSearchOpts::NextOffset(current_offset_, search_limit_);
  DoSearch();
}

std::string RadioBrowserSearchView::ActiveCountry() const {
  const char *id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(country_));
  return RadioBrowserSearchOpts::IsAllCountries(id) ? std::string() : id;
}

std::string RadioBrowserSearchView::ActiveOrder() const {
  const char *id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(sort_));
  return id ? id : RadioBrowserSearchOpts::DefaultSort();
}

void RadioBrowserSearchView::DoSearch() {
  if (!services_) {
    return;
  }
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry_));
  const std::string query = text ? text : "";
  gtk_label_set_text(GTK_LABEL(status_), Translations::CStr(RadioBrowserSearchOpts::SearchingText()));
  gtk_widget_set_visible(help_, FALSE);
  const uint64_t gen = ++search_generation_;
  services_->SearchRadioBrowser(query, ActiveCountry(), ActiveOrder(), search_limit_, current_offset_, hide_broken_,
                                [this, gen](const std::vector<RadioChannel> &channels, bool has_more, const std::string &error) {
                                  if (gen != search_generation_) {
                                    return;
                                  }
                                  if (!error.empty()) {
                                    gtk_label_set_text(GTK_LABEL(status_), error.c_str());
                                    gtk_widget_set_visible(load_more_, FALSE);
                                    return;
                                  }
                                  has_more_ = has_more;
                                  if (current_offset_ == 0) {
                                    model_.SetResults(channels);
                                  } else {
                                    model_.AddChannels(channels);
                                  }
                                  gtk_label_set_text(GTK_LABEL(status_),
                                                     RadioBrowserSearchOpts::StatusText(model_.row_count(), model_.row_count() == 0 &&
                                                                                                               current_offset_ == 0)
                                                         .c_str());
                                  gtk_widget_set_visible(load_more_, has_more_);
                                  gtk_widget_set_visible(help_, model_.row_count() == 0);
                                  ReloadResults();
                                });
}

void RadioBrowserSearchView::FetchCountries() {
  if (!services_) {
    return;
  }
  services_->FetchCountries([this](const std::vector<RadioBrowserService::Country> &countries) { ApplyCountries(countries); });
}

void RadioBrowserSearchView::ApplyCountries(const std::vector<RadioBrowserService::Country> &countries) {
  applying_countries_ = true;
  countries_loaded_ = true;
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(country_));
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(country_), RadioBrowserSearchOpts::AllCountriesId(),
                            Translations::CStr(RadioBrowserSearchOpts::AllCountriesLabel()));
  int active = 0;
  int index = 1;
  for (const RadioBrowserService::Country &country : countries) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(country_), country.code.c_str(), country.name.c_str());
    if (!default_country_.empty() && default_country_ == country.code) {
      active = index;
    }
    ++index;
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(country_), active);
  applying_countries_ = false;
}

void RadioBrowserSearchView::ReloadResults() {
  GtkWidget *child = gtk_widget_get_first_child(list_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list_), child);
    child = next;
  }
  for (const RadioChannel &channel : model_.results()) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(RadioBrowserSearchModel::RowSummary(channel).c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 12);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    auto *copy = new RadioChannel(channel);
    g_object_set_data_full(G_OBJECT(row), "channel", copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
    SetupRowDrag(row, channel);
    gtk_list_box_append(GTK_LIST_BOX(list_), row);
  }
}

void RadioBrowserSearchView::SetupRowDrag(GtkWidget *row, const RadioChannel &channel) {
  GtkDragSource *src = gtk_drag_source_new();
  gtk_drag_source_set_actions(src, GDK_ACTION_COPY);
  auto *copy = new RadioChannel(channel);
  g_object_set_data_full(G_OBJECT(src), "channel", copy, [](gpointer p) { delete static_cast<RadioChannel *>(p); });
  g_signal_connect(src, "prepare", G_CALLBACK((+[](GtkDragSource *s, double, double, gpointer data) -> GdkContentProvider * {
                     auto *self = static_cast<RadioBrowserSearchView *>(data);
                     auto *dragged = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(s), "channel"));
                     std::vector<RadioChannel> channels = dragged ? std::vector<RadioChannel>{*dragged} : std::vector<RadioChannel>{};
                     for (const RadioChannel &selected : self->SelectedChannels()) {
                       if (dragged && selected.url == dragged->url) {
                         channels = self->SelectedChannels();
                         break;
                       }
                     }
                     const std::string payload = RadioDrag::DragPayload(channels);
                     if (payload.empty()) {
                       return nullptr;
                     }
                     GValue v = G_VALUE_INIT;
                     g_value_init(&v, G_TYPE_STRING);
                     g_value_set_string(&v, payload.c_str());
                     GdkContentProvider *provider = gdk_content_provider_new_for_value(&v);
                     g_value_unset(&v);
                     return provider;
                   })),
                   this);
  gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(src));
}

std::vector<RadioChannel> RadioBrowserSearchView::SelectedChannels() const {
  std::vector<RadioChannel> channels;
  gtk_list_box_selected_foreach(
      GTK_LIST_BOX(list_),
      [](GtkListBox *, GtkListBoxRow *row, gpointer data) {
        if (auto *channel = static_cast<RadioChannel *>(g_object_get_data(G_OBJECT(row), "channel"))) {
          static_cast<std::vector<RadioChannel> *>(data)->push_back(*channel);
        }
      },
      &channels);
  return channels;
}
