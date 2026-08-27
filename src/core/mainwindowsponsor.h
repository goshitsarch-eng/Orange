#ifndef STRAWBERRY_MAINWINDOWSPONSOR_H
#define STRAWBERRY_MAINWINDOWSPONSOR_H

#include "core/mainwindowsettings.h"
#include "core/settings.h"

#include <string>

namespace MainWindowSponsor {

inline bool ShouldShow(bool do_not_show) { return !do_not_show; }

inline const char *Title() { return "Sponsoring Strawberry"; }
inline const char *WebsiteUrl() { return "https://www.strawberrymusicplayer.org/"; }
inline const char *WebsiteLabel() { return "www.strawberrymusicplayer.org"; }
inline const char *DoNotShowAgain() { return "Do not show this message again."; }

inline std::string Message() {
  return std::string(
             "Strawberry is free and open source software. If you like Strawberry, please consider sponsoring the project. For more information about sponsorship see our website ") +
         WebsiteUrl();
}

inline bool ShouldShowFromSettings(Settings &settings) {
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  return ShouldShow(settings.BoolValue(MainWindowSettings::kDoNotShowSponsorMessage, MainWindowSettings::kDefaultDoNotShowSponsorMessage));
}

inline void PersistDoNotShow(Settings &settings, bool do_not_show) {
  settings.BeginGroup(MainWindowSettings::kSettingsGroup);
  settings.SetBoolValue(MainWindowSettings::kDoNotShowSponsorMessage, do_not_show);
  settings.Sync();
}

}  // namespace MainWindowSponsor

#endif
