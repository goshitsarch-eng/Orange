#ifndef STRAWBERRY_WAVEFORMSETTINGSPAGE_H
#define STRAWBERRY_WAVEFORMSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace WaveformSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
