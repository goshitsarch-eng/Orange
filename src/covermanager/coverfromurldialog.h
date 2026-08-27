#ifndef STRAWBERRY_COVERFROMURLDIALOG_H
#define STRAWBERRY_COVERFROMURLDIALOG_H

#include "utilities/strutils.h"

#include <gtk/gtk.h>

#include <string>

class Application;

class CoverFromUrlDialog {
 public:
  static void Show(GtkWindow *parent, Application *app);
  static std::string PrefillUrl(const std::string &clipboard_text) {
    std::string text = StrUtils::Trim(clipboard_text);
    if (!text.empty() && (text.back() == '\r' || text.back() == '\n')) {
      text.pop_back();
    }
    if (text.rfind("https://", 0) == 0 || text.rfind("http://", 0) == 0) {
      return text;
    }
    return {};
  }
};

#endif
