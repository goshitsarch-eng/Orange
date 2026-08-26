#ifndef STRAWBERRY_BEHAVIOURSETTINGSPAGE_H
#define STRAWBERRY_BEHAVIOURSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace BehaviourSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
