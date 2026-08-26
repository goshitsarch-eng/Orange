#ifndef STRAWBERRY_LANGUAGECHOICES_H
#define STRAWBERRY_LANGUAGECHOICES_H

#include "translations/translations.h"

#include <string>
#include <utility>
#include <vector>

namespace LanguageChoices {

inline std::string DisplayName(const std::string &code) {
  if (code == "ca_ES") return "Català (ca_ES)";
  if (code == "cs_CZ") return "Čeština (cs_CZ)";
  if (code == "de_DE") return "Deutsch (de_DE)";
  if (code == "el_CY") return "Ελληνικά (el_CY)";
  if (code == "el_GR") return "Ελληνικά (el_GR)";
  if (code == "en_US") return "English (en_US)";
  if (code == "es_AR") return "Español (es_AR)";
  if (code == "es_ES") return "Español (es_ES)";
  if (code == "es_MX") return "Español (es_MX)";
  if (code == "et_EE") return "Eesti (et_EE)";
  if (code == "fi_FI") return "Suomi (fi_FI)";
  if (code == "fr_BE") return "Français (fr_BE)";
  if (code == "fr_FR") return "Français (fr_FR)";
  if (code == "hu_HU") return "Magyar (hu_HU)";
  if (code == "id_ID") return "Bahasa Indonesia (id_ID)";
  if (code == "is_IS") return "Íslenska (is_IS)";
  if (code == "it_IT") return "Italiano (it_IT)";
  if (code == "ja_JP") return "日本語 (ja_JP)";
  if (code == "ko_KR") return "한국어 (ko_KR)";
  if (code == "nb_NO") return "Norsk bokmål (nb_NO)";
  if (code == "nl_NL") return "Nederlands (nl_NL)";
  if (code == "pl_PL") return "Polski (pl_PL)";
  if (code == "pt_BR") return "Português (pt_BR)";
  if (code == "ru_RU") return "Русский (ru_RU)";
  if (code == "sv_SE") return "Svenska (sv_SE)";
  if (code == "tr_CY") return "Türkçe (tr_CY)";
  if (code == "tr_TR") return "Türkçe (tr_TR)";
  if (code == "uk_UA") return "Українська (uk_UA)";
  if (code == "vi_VN") return "Tiếng Việt (vi_VN)";
  if (code == "zh_CN") return "简体中文 (zh_CN)";
  if (code == "zh_TW") return "繁體中文 (zh_TW)";
  return code.empty() ? "Use the system default" : code;
}

inline std::vector<std::pair<std::string, std::string>> All() {
  std::vector<std::pair<std::string, std::string>> choices;
  choices.emplace_back("", DisplayName({}));
  for (const auto &code : Translations::AvailableLanguages()) {
    choices.emplace_back(code, DisplayName(code));
  }
  return choices;
}

}  // namespace LanguageChoices

#endif
