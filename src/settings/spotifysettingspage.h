#ifndef STRAWBERRY_SPOTIFYSETTINGSPAGE_H
#define STRAWBERRY_SPOTIFYSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace SpotifySettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
