#include "translations/translations.h"

#include "constants/behavioursettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <locale.h>

namespace {

const char *kLanguages[] = {"ca_ES", "cs_CZ", "de_DE", "el_CY", "el_GR", "en_US", "es_AR", "es_ES", "es_MX", "et_EE",
                            "fi_FI", "fr_BE", "fr_FR", "hu_HU", "id_ID", "is_IS", "it_IT", "ja_JP", "ko_KR", "nb_NO",
                            "nl_NL", "pl_PL", "pt_BR", "ru_RU", "sv_SE", "tr_CY", "tr_TR", "uk_UA", "vi_VN", "zh_CN",
                            "zh_TW"};

}  // namespace

void Translations::ApplySavedLanguage() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const std::string language = settings.Value(BehaviourSettings::kLanguage);
  if (language.empty()) {
    return;
  }
  g_setenv("LANGUAGE", language.c_str(), TRUE);
  setlocale(LC_MESSAGES, language.c_str());
}

void Translations::Init(const std::string &locale_dir) {
  const std::string dir = locale_dir.empty() ? StandardPaths::LocaleDir() : locale_dir;
  bindtextdomain("strawberry", dir.c_str());
  bind_textdomain_codeset("strawberry", "UTF-8");
  textdomain("strawberry");
}

std::string Translations::Tr(const std::string &text) { return gettext(text.c_str()); }

const char *Translations::CStr(const char *text) { return gettext(text ? text : ""); }

std::vector<std::string> Translations::AvailableLanguages() {
  return {std::begin(kLanguages), std::end(kLanguages)};
}
