#include "dialogs/addstreamdialog.h"

#include "dialogs/addstreamurl.h"
#include "translations/translations.h"

#include <adwaita.h>

void AddStreamDialog::Show(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(AddStreamUrl::Title()), Translations::CStr(AddStreamUrl::Prompt())));
  GtkWidget *url = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(url), "https://");
  adw_alert_dialog_set_extra_child(dialog, url);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "add", Translations::CStr(AddStreamUrl::Add()), nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "add", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_response_enabled(dialog, "add", FALSE);
  g_signal_connect(url, "changed", G_CALLBACK((+[](GtkEditable *editable, gpointer data) {
                     const char *text = gtk_editable_get_text(editable);
                     adw_alert_dialog_set_response_enabled(ADW_ALERT_DIALOG(data), "add",
                                                           AddStreamUrl::IsComplete(text ? text : "") ? TRUE : FALSE);
                   })),
                   dialog);
  auto *cb = new std::function<void(const std::string &, const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "url-entry", url);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &, const std::string &)> *>(data);
                     if (g_strcmp0(response, "add") == 0) {
                       GtkWidget *url_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "url-entry"));
                       const char *text = gtk_editable_get_text(GTK_EDITABLE(url_entry));
                       const std::string url_text = text ? text : "";
                       if (AddStreamUrl::IsValid(url_text)) {
                         (*fn)({}, url_text);
                       }
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
