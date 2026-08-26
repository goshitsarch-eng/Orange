#ifndef STRAWBERRY_ORGANIZETOKENHELP_H
#define STRAWBERRY_ORGANIZETOKENHELP_H

#include <cstring>

namespace OrganizeTokenHelp {

inline const char *Tooltip() {
  return "Tokens start with %, for example: %artist %album %title\n\n"
         "If you surround sections of text that contain a token with curly-braces, that section will be hidden if the token is empty.";
}

inline bool MentionsTokenExample(const char *text) { return text && std::strstr(text, "%artist") && std::strstr(text, "%album") && std::strstr(text, "%title"); }

inline bool MentionsOptionalBraces(const char *text) { return text && std::strstr(text, "curly-braces"); }

}  // namespace OrganizeTokenHelp

#endif  // STRAWBERRY_ORGANIZETOKENHELP_H
