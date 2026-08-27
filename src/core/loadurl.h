#ifndef STRAWBERRY_LOADURL_H
#define STRAWBERRY_LOADURL_H

#include "core/commandlineurl.h"
#include "tidal/tidalloginurl.h"

#include <string>

namespace LoadUrl {

// Qt MainWindow::LoadUrl: existing local path, tidal://login, otherwise reject.
enum class Action {
  InsertLocal = 0,
  TidalLogin = 1,
  Reject = 2
};

inline Action Resolve(const std::string &url) {
  if (CommandlineUrl::LocalFileExists(url)) {
    return Action::InsertLocal;
  }
  if (TidalLoginUrl::IsLogin(url)) {
    return Action::TidalLogin;
  }
  return Action::Reject;
}

inline bool ShouldInsert(const std::string &url) { return Resolve(url) != Action::Reject; }

inline const char *RejectMessage() { return "Can't open"; }

inline std::string MissingFileMessage(const std::string &path) { return "File " + path + " does not exist."; }

}  // namespace LoadUrl

#endif
