#ifndef STRAWBERRY_LYRICSPROVIDERAUTH_H
#define STRAWBERRY_LYRICSPROVIDERAUTH_H

#include <string>

namespace LyricsProviderAuth {

enum class Panel { Hidden, Direct };

inline bool RequiresAuthentication(const std::string &name) { return name == "Genius"; }

inline const char *AuthenticationGroup() { return "Authentication"; }

inline const char *NoProviderSelected() { return "No provider selected."; }

inline const char *Login() { return "Login"; }

// Qt LyricsSettingsPage::CurrentItemChanged — Genius is the only lyrics provider that authenticates.
inline Panel PanelFor(const std::string &name, bool authentication_required) {
  if (name.empty() || !authentication_required) {
    return Panel::Hidden;
  }
  return Panel::Direct;
}

inline bool AuthenticateVisible(Panel panel) { return panel == Panel::Direct; }

inline bool AuthenticateEnabled(Panel panel, bool login_in_progress) { return panel == Panel::Direct && !login_in_progress; }

inline bool LoginStateVisible(Panel panel) { return panel == Panel::Direct; }

inline bool CredentialsVisible(Panel panel) { return panel == Panel::Direct; }

inline std::string SelectionStatusText(const std::string &name, bool authentication_required) {
  if (name.empty()) {
    return NoProviderSelected();
  }
  if (!authentication_required) {
    return name + " does not need authentication.";
  }
  return name + " needs authentication.";
}

inline bool MoveUpEnabled(int row, int count) { return row > 0 && count > 0; }

inline bool MoveDownEnabled(int row, int count) { return row >= 0 && row + 1 < count; }

}  // namespace LyricsProviderAuth

#endif
