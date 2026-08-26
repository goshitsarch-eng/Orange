#ifndef STRAWBERRY_TRANSLATIONS_H
#define STRAWBERRY_TRANSLATIONS_H

#include <string>
#include <vector>

class Translations {
 public:
  static void Init(const std::string &locale_dir = {});
  static std::string Tr(const std::string &text);
  static const char *CStr(const char *text);
  static std::vector<std::string> AvailableLanguages();
  static void ApplySavedLanguage();
};

#endif
