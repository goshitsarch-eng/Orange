#ifndef STRAWBERRY_SETTINGSGROUPSCOPE_H
#define STRAWBERRY_SETTINGSGROUPSCOPE_H

#include "core/settings.h"

#include <glib-object.h>

// Remembering which settings group a widget belongs to.
//
// One Settings object is shared by every page of the preferences dialog, and it carries a single "current
// group" that each page moves when it is built. A widget's callback runs long after that - when the user
// actually changes something - by which point the current group belongs to whichever page happened to be
// built last. A widget therefore has to record the group it was created under and restore it before writing,
// or the value lands in the wrong section of the settings file and is silently lost.
namespace SettingsGroupScope {

inline constexpr const char *kKey = "settings-group";

// Records the group this widget's value belongs to: the one named explicitly, or the one in effect now.
inline void Stash(gpointer object, Settings *settings, const char *group_name) {
  const char *effective = group_name;
  if (!effective && settings) {
    effective = settings->group().c_str();
  }
  if (effective && *effective) {
    g_object_set_data_full(G_OBJECT(object), kKey, g_strdup(effective), g_free);
  }
}

// Puts the settings object back into the group the widget was created under.
inline void Restore(gpointer object, Settings *settings) {
  if (!settings) {
    return;
  }
  const auto *group_name = static_cast<const char *>(g_object_get_data(G_OBJECT(object), kKey));
  if (group_name) {
    settings->BeginGroup(group_name);
  }
}

}  // namespace SettingsGroupScope

#endif  // STRAWBERRY_SETTINGSGROUPSCOPE_H
