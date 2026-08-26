#include "radios/radioviewcontainer.h"

#include "radios/radioservices.h"

RadioViewContainer::RadioViewContainer(RadioServices *services) : services_(services) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  search_view_ = std::make_unique<RadioBrowserSearchView>();
  view_ = std::make_unique<RadioView>();
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), search_view_->widget());
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  search_view_->SetChangedCallback([this](const std::string &query) { Search(query); });
  Reload();
}

void RadioViewContainer::Reload() {
  if (!services_) {
    return;
  }
  if (!search_view_->model()->results().empty()) {
    model_.SetSearchResults(search_view_->model()->results());
  } else {
    if (services_->channels().empty()) {
      services_->FetchSomaFM();
      services_->FetchRadioParadise();
    }
    model_.SetChannels(services_->channels());
  }
  view_->Reload(&model_);
}

void RadioViewContainer::Search(const std::string &query) {
  if (!services_) {
    return;
  }
  if (query.empty()) {
    search_view_->SetResults({});
    Reload();
    return;
  }
  services_->FetchRadioBrowser(query);
  search_view_->SetResults(services_->search_results());
  Reload();
}

void RadioViewContainer::SetActivateCallback(std::function<void(const RadioChannel &)> callback) {
  view_->SetActivateCallback(std::move(callback));
}
