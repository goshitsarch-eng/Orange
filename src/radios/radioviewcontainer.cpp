#include "radios/radioviewcontainer.h"

#include "radios/radiobrowsersearchopts.h"
#include "radios/radioservices.h"
#include "radios/radioviewsearch.h"
#include "radios/radioviewshow.h"
#include "translations/translations.h"

RadioViewContainer::RadioViewContainer(RadioServices *services) : services_(services) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  search_view_ = std::make_unique<RadioBrowserSearchView>(services_);
  view_ = std::make_unique<RadioView>();
  gtk_widget_set_vexpand(view_->widget(), TRUE);

  GtkWidget *channels = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 4);
  gtk_widget_set_margin_bottom(toolbar, 4);
  GtkWidget *refresh = gtk_button_new_from_icon_name("view-refresh-symbolic");
  gtk_widget_set_tooltip_text(refresh, Translations::CStr("Refresh channels"));
  gtk_box_append(GTK_BOX(toolbar), refresh);
  gtk_box_append(GTK_BOX(channels), toolbar);
  gtk_box_append(GTK_BOX(channels), view_->widget());
  g_signal_connect(refresh, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<RadioViewContainer *>(data)->RefreshChannels();
                   }),
                   this);
  view_->SetRefreshCallback([this]() { RefreshChannels(); });

  stack_ = gtk_stack_new();
  gtk_widget_set_vexpand(stack_, TRUE);
  gtk_stack_add_titled(GTK_STACK(stack_), channels, RadioViewSearch::ChannelsTabId(), Translations::CStr(RadioBrowserSearchOpts::ChannelsTab()));
  gtk_stack_add_titled(GTK_STACK(stack_), search_view_->widget(), RadioViewSearch::BrowserTabId(),
                       Translations::CStr(RadioBrowserSearchOpts::BrowserTab()));
  GtkWidget *switcher = gtk_stack_switcher_new();
  gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(switcher, 4);
  gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack_));
  gtk_box_append(GTK_BOX(widget_), switcher);
  gtk_box_append(GTK_BOX(widget_), stack_);
  Reload();
  g_signal_connect(view_->widget(), "map", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                     static_cast<RadioViewContainer *>(data)->OnShown();
                   }),
                   this);
}

void RadioViewContainer::OnShown() {
  if (!RadioViewShow::ShouldFetchOnShow(channels_initialized_)) {
    return;
  }
  channels_initialized_ = true;
  Reload();
  RefreshChannels();
}

void RadioViewContainer::Reload() {
  if (!services_) {
    return;
  }
  model_.SetChannels(services_->channels());
  view_->Reload(&model_);
}

void RadioViewContainer::RefreshChannels() {
  if (!services_) {
    return;
  }
  services_->FetchSomaFM();
  services_->FetchRadioParadise();
}

void RadioViewContainer::Search(const std::string &query) {
  if (stack_ && RadioViewSearch::ShouldShowBrowserTab()) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack_), RadioViewSearch::BrowserTabId());
  }
  if (search_view_ && RadioViewSearch::ShouldRunSearch()) {
    search_view_->Search(query);
  }
}

void RadioViewContainer::SetActivateCallback(std::function<void(const RadioChannel &)> callback) {
  view_->SetActivateCallback(callback);
  search_view_->SetActivateCallback(std::move(callback));
}

void RadioViewContainer::SetEnqueueCallback(RadioView::EnqueueCallback callback) { view_->SetEnqueueCallback(std::move(callback)); }

void RadioViewContainer::SetMenuCallback(RadioView::MenuCallback callback) {
  view_->SetMenuCallback(callback);
  search_view_->SetMenuCallback(std::move(callback));
}
