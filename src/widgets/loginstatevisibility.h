#ifndef STRAWBERRY_LOGINSTATEVISIBILITY_H
#define STRAWBERRY_LOGINSTATEVISIBILITY_H

#include "widgets/loginstatewidget.h"

#include <string>

namespace LoginStateVisibility {

inline const char *SignedIn() { return "You are signed in."; }
inline const char *SignedOut() { return "You are not signed in."; }
inline const char *SigningIn() { return "Signing in..."; }
inline const char *SignOut() { return "Sign out"; }

inline std::string SignedInAs(const std::string &account) {
  if (account.empty()) {
    return SignedIn();
  }
  return "You are signed in as " + account + ".";
}

inline bool ShowCredentials(LoginStateWidget::State state) { return state != LoginStateWidget::State::LoggedIn; }

inline bool CredentialsEnabled(LoginStateWidget::State state) { return state != LoginStateWidget::State::LoginInProgress; }

inline const char *StatusText(LoginStateWidget::State state) {
  switch (state) {
    case LoginStateWidget::State::LoggedIn:
      return SignedIn();
    case LoginStateWidget::State::LoginInProgress:
      return SigningIn();
    case LoginStateWidget::State::LoggedOut:
    default:
      return SignedOut();
  }
}

inline bool ShowLogin(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoggedOut; }

inline bool ShowLogout(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoggedIn; }

inline bool ShowProgress(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoginInProgress; }

}  // namespace LoginStateVisibility

#endif
