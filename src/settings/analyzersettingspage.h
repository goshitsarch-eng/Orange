#ifndef STRAWBERRY_ANALYZERSETTINGSPAGE_H
#define STRAWBERRY_ANALYZERSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace AnalyzerSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
