#ifndef STRAWBERRY_CONTEXTPLAYINGTEXT_H
#define STRAWBERRY_CONTEXTPLAYINGTEXT_H

#include <string>

namespace ContextPlayingText {

inline std::string EscapeMarkup(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (char ch : text) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out += ch;
        break;
    }
  }
  return out;
}

inline std::string TopMarkup(const std::string &title, const std::string &summary) {
  std::string markup = "<b>" + EscapeMarkup(title) + "</b>";
  if (!summary.empty()) {
    markup += "\n" + EscapeMarkup(summary);
  }
  return markup;
}

}  // namespace ContextPlayingText

#endif  // STRAWBERRY_CONTEXTPLAYINGTEXT_H
