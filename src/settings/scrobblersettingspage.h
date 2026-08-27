#ifndef STRAWBERRY_SCROBBLERSETTINGSPAGE_H
#define STRAWBERRY_SCROBBLERSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace ScrobblerSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
