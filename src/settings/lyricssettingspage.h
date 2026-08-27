#ifndef STRAWBERRY_LYRICSSETTINGSPAGE_H
#define STRAWBERRY_LYRICSSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace LyricsSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
