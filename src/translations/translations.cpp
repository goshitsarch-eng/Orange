#include "translations/translations.h"

#include <glib/gi18n.h>
#include <glib.h>

void Translations::Init(const std::string &locale_dir) {
  bindtextdomain("strawberry", locale_dir.empty() ? "/usr/share/locale" : locale_dir.c_str());
  bind_textdomain_codeset("strawberry", "UTF-8");
  textdomain("strawberry");
}

std::string Translations::Tr(const std::string &text) { return gettext(text.c_str()); }

std::vector<std::string> Translations::AvailableLanguages() { return {"en", "de", "fr", "es", "it", "nl", "pl", "pt", "ru", "zh"}; }
