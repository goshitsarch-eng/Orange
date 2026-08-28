#include "dialogs/aboutdialog.h"

#include "dialogs/aboutcredits.h"
#include "dialogs/dialogclosekeys.h"
#include "translations/translations.h"
#include "version.h"

#include <adwaita.h>

void AboutDialog::Show(GtkWindow *parent) {
  AdwDialog *about = adw_about_dialog_new();
  adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), "Strawberry");
  adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about), "strawberry");
  adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), STRAWBERRY_VERSION_DISPLAY);
  adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about), AboutCredits::AuthorName());
  adw_about_dialog_set_developers(ADW_ABOUT_DIALOG(about), AboutCredits::Developers());
  adw_about_dialog_set_copyright(ADW_ABOUT_DIALOG(about), AboutCredits::Copyright());
  adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about), GTK_LICENSE_GPL_3_0);
  adw_about_dialog_set_comments(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::Comments()));
  adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), AboutCredits::Website());
  adw_about_dialog_set_issue_url(ADW_ABOUT_DIALOG(about), AboutCredits::SourceUrl());
  adw_about_dialog_set_translator_credits(ADW_ABOUT_DIALOG(about), Translations::CStr("translator-credits"));

  adw_about_dialog_add_link(ADW_ABOUT_DIALOG(about), Translations::CStr("Source code"), AboutCredits::SourceUrl());
  adw_about_dialog_add_link(ADW_ABOUT_DIALOG(about), Translations::CStr("Sponsor on Patreon"), AboutCredits::PatreonUrl());
  adw_about_dialog_add_link(ADW_ABOUT_DIALOG(about), Translations::CStr("Sponsor on GitHub"), AboutCredits::GitHubSponsorsUrl());
  adw_about_dialog_add_link(ADW_ABOUT_DIALOG(about), Translations::CStr("Donate via PayPal"), AboutCredits::PayPalUrl());

  adw_about_dialog_add_credit_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::ContributorsSection()),
                                      AboutCredits::StrawberryContributors());
  adw_about_dialog_add_credit_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::ClementineAuthorsSection()),
                                      AboutCredits::ClementineAuthors());
  adw_about_dialog_add_credit_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::ClementineContributorsSection()),
                                      AboutCredits::ClementineContributors());
  // Strawberry forked Clementine, which was itself a port of Amarok 1.4.
  adw_about_dialog_add_credit_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::AmarokSection()),
                                      AboutCredits::AmarokAuthors());
  adw_about_dialog_add_acknowledgement_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::ThanksSection()),
                                               AboutCredits::ThanksTo());
  adw_about_dialog_add_acknowledgement_section(ADW_ABOUT_DIALOG(about), Translations::CStr(AboutCredits::ThirdPartySection()),
                                               AboutCredits::ThirdParty());
  DialogCloseKeys::Attach(about);
  adw_dialog_present(about, GTK_WIDGET(parent));
}
