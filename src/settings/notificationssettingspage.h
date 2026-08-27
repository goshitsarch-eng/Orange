#ifndef STRAWBERRY_NOTIFICATIONSSETTINGSPAGE_H
#define STRAWBERRY_NOTIFICATIONSSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace NotificationsSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
