#ifndef STRAWBERRY_COLLECTIONSETTINGSPAGE_H
#define STRAWBERRY_COLLECTIONSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace CollectionSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
