#include "covermanager/coverfromurldialog.h"

#include "core/application.h"
#include "covermanager/coverfromurllabels.h"
#include "covermanager/coverproviders.h"
#include "translations/translations.h"
#include "utilities/jsonutils.h"

#include <adwaita.h>

void CoverFromUrlDialog::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(CoverFromUrlLabels::Title()));
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *prompt = gtk_label_new(Translations::CStr(CoverFromUrlLabels::Prompt()));
  gtk_label_set_wrap(GTK_LABEL(prompt), TRUE);
  gtk_widget_set_halign(prompt, GTK_ALIGN_START);
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "https://");
  GtkWidget *spinner = gtk_spinner_new();
  gtk_widget_set_visible(spinner, FALSE);
  GtkWidget *status = gtk_label_new(Translations::CStr("Download artwork and save it next to the current song."));
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *fetch = gtk_button_new_with_label(Translations::CStr("Download"));
  gtk_widget_add_css_class(fetch, "suggested-action");
  g_object_set_data(G_OBJECT(fetch), "entry", entry);
  g_object_set_data(G_OBJECT(fetch), "spinner", spinner);
  g_object_set_data(G_OBJECT(fetch), "status", status);
  g_signal_connect(fetch, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     const char *url = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "entry")));
                     GtkWidget *spin = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "spinner"));
                     GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                     const std::string request = PrefillUrl(url ? url : "");
                     if (request.empty()) {
                       gtk_label_set_text(GTK_LABEL(label), Translations::CStr("Enter an http or https image URL."));
                       return;
                     }
                     gtk_widget_set_visible(spin, TRUE);
                     gtk_spinner_start(GTK_SPINNER(spin));
                     gtk_label_set_text(GTK_LABEL(label), Translations::CStr("Downloading…"));
                     gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
                     application->network()->Get(request, [button, spin, label, application](const NetworkAccessManager::Response &result) {
                       gtk_spinner_stop(GTK_SPINNER(spin));
                       gtk_widget_set_visible(spin, FALSE);
                       gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
                       if (!result.ok()) {
                         gtk_label_set_text(GTK_LABEL(label), Translations::CStr("The site you requested does not exist!"));
                         return;
                       }
                       if (!JsonUtils::LooksLikeImage(result.body)) {
                         gtk_label_set_text(GTK_LABEL(label), Translations::CStr("The site you requested is not an image!"));
                         return;
                       }
                       if (CoverProviders::SaveAlbumCover(application->player()->current_song(), result.body, application->tagreader())) {
                         gtk_label_set_text(GTK_LABEL(label), Translations::CStr("Cover saved."));
                       } else {
                         gtk_label_set_text(GTK_LABEL(label), Translations::CStr("Could not save cover."));
                       }
                     });
                   }),
                   app);
  if (GdkDisplay *display = gdk_display_get_default()) {
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);
    gdk_clipboard_read_text_async(clipboard, nullptr, +[](GObject *source, GAsyncResult *res, gpointer data) {
      char *text = gdk_clipboard_read_text_finish(GDK_CLIPBOARD(source), res, nullptr);
      if (text) {
        const std::string url = PrefillUrl(text);
        if (!url.empty()) {
          gtk_editable_set_text(GTK_EDITABLE(data), url.c_str());
        }
        g_free(text);
      }
    }, entry);
  }
  gtk_box_append(GTK_BOX(box), prompt);
  gtk_box_append(GTK_BOX(box), entry);
  gtk_box_append(GTK_BOX(box), spinner);
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), fetch);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
