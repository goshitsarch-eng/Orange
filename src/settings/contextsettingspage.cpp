#include "settings/contextsettingspage.h"

#include "constants/contextsettings.h"
#include "settings/contextsettingslabels.h"
#include "context/contextcover.h"
#include "context/contextfont.h"
#include "context/contextfontcontrols.h"
#include "context/contextfontpreview.h"
#include "context/contextformattokens.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "utilities/fontutils.h"

#include <pango/pango.h>

namespace {

void ApplyPreview(GtkWidget *label, const FontUtils::Font &font) {
  if (!label) {
    return;
  }
  PangoFontDescription *desc = pango_font_description_from_string(ContextFontPreview::Pango(font).c_str());
  PangoAttrList *attrs = pango_attr_list_new();
  pango_attr_list_insert(attrs, pango_attr_font_desc_new(desc));
  gtk_label_set_attributes(GTK_LABEL(label), attrs);
  pango_attr_list_unref(attrs);
  pango_font_description_free(desc);
}

}  // namespace

AdwPreferencesPage *ContextSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(ContextSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Context", "view-paged-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, ContextSettingsLabels::EnableItems());
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsTitleFmt, ContextSettingsLabels::Title(), ContextSettings::kDefaultTitleFmt);
  SettingsPage::AddEntry(group, settings, ContextSettings::kSettingsSummaryFmt, ContextSettingsLabels::Summary(), ContextSettings::kDefaultSummaryFmt);
  AdwPreferencesGroup *title_tokens = SettingsPage::AddGroup(page, "Insert into title");
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(title_tokens, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(ContextSettings::kSettingsGroup);
      const std::string current = settings->Value(ContextSettings::kSettingsTitleFmt, ContextSettings::kDefaultTitleFmt);
      settings->SetValue(ContextSettings::kSettingsTitleFmt, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  AdwPreferencesGroup *summary_tokens = SettingsPage::AddGroup(page, "Insert into summary");
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(summary_tokens, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(ContextSettings::kSettingsGroup);
      const std::string current = settings->Value(ContextSettings::kSettingsSummaryFmt, ContextSettings::kDefaultSummaryFmt);
      settings->SetValue(ContextSettings::kSettingsSummaryFmt, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  SettingsPage::AddToggle(group, settings, ContextSettings::kAlbum, ContextSettingsLabels::Album(), nullptr, ContextSettings::kDefaultAlbum);
  SettingsPage::AddToggle(group, settings, ContextSettings::kTechnicalData, ContextSettingsLabels::TechnicalData(), nullptr,
                          ContextSettings::kDefaultTechnicalData);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSongLyrics, ContextSettingsLabels::SongLyrics(), nullptr,
                          ContextSettings::kDefaultSongLyrics);
  AdwSwitchRow *search_cover = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(search_cover), Translations::CStr(ContextSettingsLabels::SearchCover()));
  adw_switch_row_set_active(search_cover, ContextCover::LoadEnabled(*settings));
  g_signal_connect(search_cover, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     ContextCover::PersistEnabled(*s, adw_switch_row_get_active(row));
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(search_cover));
  settings->BeginGroup(ContextSettings::kSettingsGroup);
  SettingsPage::AddToggle(group, settings, ContextSettings::kSearchLyrics, ContextSettingsLabels::SearchLyrics(), nullptr,
                          ContextSettings::kDefaultSearchLyrics);

  const FontUtils::Font headline = ContextFontControls::Headline(
      settings->Value(ContextSettings::kFontHeadline, ContextSettings::kDefaultFontFamily),
      settings->DoubleValue(ContextSettings::kFontSizeHeadline, ContextSettings::kDefaultFontSizeHeadline));
  const FontUtils::Font normal = ContextFontControls::Normal(
      settings->Value(ContextSettings::kFontNormal, ContextSettings::kDefaultFontFamily),
      settings->DoubleValue(ContextSettings::kFontSizeNormal, ContextSettings::kDefaultFontSizeNormal));
  const std::string headline_pango = FontUtils::ToPango(headline);
  const std::string normal_pango = FontUtils::ToPango(normal);

  AdwPreferencesGroup *headline_group = SettingsPage::AddGroup(page, ContextFontControls::HeadlineGroup());
  SettingsPage::AddFontButton(headline_group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontHeadline,
                              ContextFontControls::FontTitle(), headline_pango.c_str());
  GtkWidget *headline_size =
      SettingsPage::AddDoubleScale(headline_group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontSizeHeadline,
                                   ContextFontControls::SizeTitle(), ContextSettings::kDefaultFontSizeHeadline, ContextFontControls::MinPt(),
                                   ContextFontControls::MaxPt(), ContextFontControls::Step());
  GtkWidget *headline_preview = gtk_label_new(ContextFontPreview::HeadlineSample());
  gtk_label_set_wrap(GTK_LABEL(headline_preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(headline_preview), 0.0f);
  gtk_widget_set_margin_start(headline_preview, 12);
  gtk_widget_set_tooltip_text(headline_preview, Translations::CStr(ContextFontPreview::Title()));
  ApplyPreview(headline_preview, headline);
  g_object_set_data(G_OBJECT(headline_size), "preview", headline_preview);
  g_object_set_data(G_OBJECT(headline_size), "settings", settings);
  g_signal_connect(headline_size, "value-changed", G_CALLBACK(+[](GtkRange *, gpointer data) {
                     auto *label = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "preview"));
                     auto *s = static_cast<Settings *>(g_object_get_data(G_OBJECT(data), "settings"));
                     if (!label || !s) {
                       return;
                     }
                     s->BeginGroup(ContextSettings::kSettingsGroup);
                     ApplyPreview(label, ContextFontControls::Headline(
                                             s->Value(ContextSettings::kFontHeadline, ContextSettings::kDefaultFontFamily),
                                             s->DoubleValue(ContextSettings::kFontSizeHeadline, ContextSettings::kDefaultFontSizeHeadline)));
                   }),
                   headline_size);
  adw_preferences_group_add(headline_group, headline_preview);

  AdwPreferencesGroup *normal_group = SettingsPage::AddGroup(page, ContextFontControls::NormalGroup());
  SettingsPage::AddFontButton(normal_group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontNormal,
                              ContextFontControls::FontTitle(), normal_pango.c_str());
  GtkWidget *normal_size =
      SettingsPage::AddDoubleScale(normal_group, settings, ContextSettings::kSettingsGroup, ContextSettings::kFontSizeNormal,
                                   ContextFontControls::SizeTitle(), ContextSettings::kDefaultFontSizeNormal, ContextFontControls::MinPt(),
                                   ContextFontControls::MaxPt(), ContextFontControls::Step());
  GtkWidget *normal_preview = gtk_label_new(ContextFontPreview::NormalSample());
  gtk_label_set_wrap(GTK_LABEL(normal_preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(normal_preview), 0.0f);
  gtk_widget_set_margin_start(normal_preview, 12);
  gtk_widget_set_tooltip_text(normal_preview, Translations::CStr(ContextFontPreview::Title()));
  ApplyPreview(normal_preview, normal);
  g_object_set_data(G_OBJECT(normal_size), "preview", normal_preview);
  g_object_set_data(G_OBJECT(normal_size), "settings", settings);
  g_signal_connect(normal_size, "value-changed", G_CALLBACK(+[](GtkRange *, gpointer data) {
                     auto *label = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "preview"));
                     auto *s = static_cast<Settings *>(g_object_get_data(G_OBJECT(data), "settings"));
                     if (!label || !s) {
                       return;
                     }
                     s->BeginGroup(ContextSettings::kSettingsGroup);
                     ApplyPreview(label, ContextFontControls::Normal(
                                             s->Value(ContextSettings::kFontNormal, ContextSettings::kDefaultFontFamily),
                                             s->DoubleValue(ContextSettings::kFontSizeNormal, ContextSettings::kDefaultFontSizeNormal)));
                   }),
                   normal_size);
  adw_preferences_group_add(normal_group, normal_preview);
  return page;
}
