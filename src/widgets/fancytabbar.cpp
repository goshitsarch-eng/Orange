#include "widgets/fancytabbar.h"

FancyTabBar::FancyTabBar() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_add_css_class(widget_, "sidebar");
}

void FancyTabBar::AddTab(const std::string &id, const std::string &title, const std::string &icon) {
  GtkWidget *button = gtk_button_new_from_icon_name(icon.empty() ? "audio-x-generic-symbolic" : icon.c_str());
  gtk_button_set_label(GTK_BUTTON(button), title.c_str());
  gtk_widget_set_hexpand(button, TRUE);
  g_object_set_data_full(G_OBJECT(button), "tab-id", g_strdup(id.c_str()), g_free);
  g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                     auto *self = static_cast<FancyTabBar *>(data);
                     const char *tab = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "tab-id"));
                     if (tab) {
                       self->SetActive(tab);
                     }
                   }),
                   this);
  gtk_box_append(GTK_BOX(widget_), button);
}

void FancyTabBar::SetActive(const std::string &id) {
  active_ = id;
  if (activate_) {
    activate_(id);
  }
}

void FancyTabBar::SetActivateCallback(std::function<void(const std::string &)> callback) { activate_ = std::move(callback); }
