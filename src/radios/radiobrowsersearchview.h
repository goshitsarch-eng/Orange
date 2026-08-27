#ifndef STRAWBERRY_RADIOBROWSERSEARCHVIEW_H
#define STRAWBERRY_RADIOBROWSERSEARCHVIEW_H

#include "radios/radiobrowsersearchmodel.h"
#include "radios/radiobrowserservice.h"
#include "radios/radioview.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class RadioServices;

class RadioBrowserSearchView {
 public:
  explicit RadioBrowserSearchView(RadioServices *services = nullptr);
  ~RadioBrowserSearchView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *entry() const { return entry_; }
  RadioBrowserSearchModel *model() { return &model_; }
  void SetResults(const std::vector<RadioChannel> &results);
  void Search(const std::string &query);
  void SetChangedCallback(std::function<void(const std::string &)> callback) { changed_ = std::move(callback); }
  void SetActivateCallback(std::function<void(const RadioChannel &)> callback) { activate_ = std::move(callback); }
  void SetMenuCallback(RadioView::MenuCallback callback) { menu_ = std::move(callback); }
  std::vector<RadioChannel> SelectedChannels() const;

 private:
  void ReloadResults();
  void ScheduleSearch();
  void SearchTriggered();
  void DoSearch();
  void LoadMore();
  void FetchCountries();
  void ApplyCountries(const std::vector<RadioBrowserService::Country> &countries);
  std::string ActiveCountry() const;
  std::string ActiveOrder() const;
  void SetupRowDrag(GtkWidget *row, const RadioChannel &channel);
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);

  RadioServices *services_ = nullptr;
  GtkWidget *widget_ = nullptr;
  GtkWidget *entry_ = nullptr;
  GtkWidget *country_ = nullptr;
  GtkWidget *sort_ = nullptr;
  GtkWidget *status_ = nullptr;
  GtkWidget *help_ = nullptr;
  GtkWidget *list_ = nullptr;
  GtkWidget *load_more_ = nullptr;
  RadioBrowserSearchModel model_;
  std::function<void(const std::string &)> changed_;
  std::function<void(const RadioChannel &)> activate_;
  RadioView::MenuCallback menu_;
  int search_limit_ = 100;
  int current_offset_ = 0;
  bool hide_broken_ = true;
  bool has_more_ = false;
  bool countries_loaded_ = false;
  bool applying_countries_ = false;
  guint search_timeout_ = 0;
  uint64_t search_generation_ = 0;
  std::string default_country_;
};

#endif
