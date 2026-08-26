#ifndef STRAWBERRY_MOODBARSETTINGSPAGE_H
#define STRAWBERRY_MOODBARSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace MoodbarSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
