#ifndef STRAWBERRY_DISCORD_H
#define STRAWBERRY_DISCORD_H
#include "core/song.h"
class DiscordRichPresence {
 public:
  void ReloadSettings();
  void UpdatePresence(const Song &song, bool playing);
  void Clear();
  bool enabled() const { return enabled_; }
 private:
  bool enabled_ = false;
};
#endif
