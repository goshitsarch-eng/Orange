#ifndef STRAWBERRY_PLAYLISTSETTINGSPAGE_H
#define STRAWBERRY_PLAYLISTSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace PlaylistSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
