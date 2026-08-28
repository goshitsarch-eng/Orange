#ifndef STRAWBERRY_STREAMINGLOGINCONTROLS_H
#define STRAWBERRY_STREAMINGLOGINCONTROLS_H

#include <string>

namespace StreamingLoginControls {

// Qt Tidal/Spotify/QobuzSettingsPage::LoginClicked — disable while OAuth is in flight.
inline bool LoginButtonEnabled(bool auth_in_progress) { return !auth_in_progress; }

inline bool ShouldDisableOnStart(bool credentials_valid) { return credentials_valid; }

inline bool LoginButtonEnabledAfterAuth() { return true; }

// Qt eventFilter: dialog Enter re-enables Login.
inline bool LoginButtonEnabledOnPageShown() { return true; }

inline bool TidalCredentialsValid(const std::string &client_id) { return !client_id.empty(); }

// Qt Tidal/Spotify/QobuzSettingsPage::showEvent — refresh LoginStateWidget from service auth.
inline bool ShouldRefreshLoginStateOnShow() { return true; }

enum class LoginState { LoggedOut, LoginInProgress, LoggedIn };

inline LoginState StateFromAuth(bool logged_in, bool in_progress) {
  if (in_progress) {
    return LoginState::LoginInProgress;
  }
  return logged_in ? LoginState::LoggedIn : LoginState::LoggedOut;
}

}  // namespace StreamingLoginControls

#endif
