#ifndef STRAWBERRY_TRANSCODERSETTINGSPAGE_H
#define STRAWBERRY_TRANSCODERSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace TranscoderSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
