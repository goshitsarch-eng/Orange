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

}  // namespace StreamingLoginControls

#endif
