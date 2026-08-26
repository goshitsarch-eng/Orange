#include "dialogs/aboutdialog.h"

#include "translations/translations.h"
#include "version.h"

#include <adwaita.h>

void AboutDialog::Show(GtkWindow *parent) {
  AdwDialog *about = adw_about_dialog_new();
  adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), "Strawberry");
  adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about), "strawberry");
  adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), STRAWBERRY_VERSION_DISPLAY);
  adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about), Translations::CStr("Author and maintainer"));
  adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about), GTK_LICENSE_GPL_3_0);
  adw_about_dialog_set_comments(ADW_ABOUT_DIALOG(about), Translations::CStr("Strawberry is a music player and music collection organizer."));
  adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), "https://www.strawberrymusicplayer.org");
  adw_dialog_present(about, GTK_WIDGET(parent));
}
