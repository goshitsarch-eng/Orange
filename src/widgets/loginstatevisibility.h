#ifndef STRAWBERRY_LOGINSTATEVISIBILITY_H
#define STRAWBERRY_LOGINSTATEVISIBILITY_H

#include "widgets/loginstatewidget.h"

namespace LoginStateVisibility {

inline bool ShowCredentials(LoginStateWidget::State state) { return state != LoginStateWidget::State::LoggedIn; }

inline bool CredentialsEnabled(LoginStateWidget::State state) { return state != LoginStateWidget::State::LoginInProgress; }

inline const char *StatusText(LoginStateWidget::State state) {
  switch (state) {
    case LoginStateWidget::State::LoggedIn:
      return "Signed in";
    case LoginStateWidget::State::LoginInProgress:
      return "Signing in…";
    case LoginStateWidget::State::LoggedOut:
    default:
      return "Not signed in";
  }
}

inline bool ShowLogin(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoggedOut; }

inline bool ShowLogout(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoggedIn; }

inline bool ShowProgress(LoginStateWidget::State state) { return state == LoginStateWidget::State::LoginInProgress; }

}  // namespace LoginStateVisibility

#endif
