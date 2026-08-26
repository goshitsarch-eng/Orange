#include "covermanager/coverfromurldialog.h"

#include "core/application.h"
#include "covermanager/coverproviders.h"
#include "utilities/jsonutils.h"

#include <adwaita.h>

void CoverFromUrlDialog::Show(GtkWindow *parent, Application *app) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Cover from URL", "Download artwork and save it next to the current song."));
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "https://");
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "fetch", "Download", nullptr);
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "fetch") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     const char *url = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry")));
                     if (!url || !*url) {
                       return;
                     }
                     application->network()->Get(url, [application](const NetworkAccessManager::Response &result) {
                       if (result.ok() && JsonUtils::LooksLikeImage(result.body)) {
                         CoverProviders::SaveAlbumCover(application->player()->current_song(), result.body, application->tagreader());
                       }
                     });
                   }),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
