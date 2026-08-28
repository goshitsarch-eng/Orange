#ifndef STRAWBERRY_MAINWINDOWSPONSOR_H
#define STRAWBERRY_MAINWINDOWSPONSOR_H

#include "core/mainwindowsettings.h"
#include "core/settings.h"

#include <string>

namespace MainWindowSponsor {

inline bool ShouldShow(bool do_not_show) { return !do_not_show; }

inline const char *Title() { return "Supporting Orange"; }
// The sponsorship page belongs to Strawberry, the project Orange is forked from, so the prompt says whose
// work the money supports rather than implying it funds Orange.
inline const char *WebsiteUrl() { return "https://www.strawberrymusicplayer.org/"; }
inline const char *WebsiteLabel() { return "www.strawberrymusicplayer.org"; }
inline const char *DoNotShowAgain() { return "Do not show this message again."; }

inline std::string Message() {
  return std::string(
             "Orange is free and open source software, built on Strawberry. If you find it useful, please consider "
             "supporting Strawberry's author, whose work Orange is based on. For more information see ") +
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
