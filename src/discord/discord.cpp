#include "discord/discord.h"
#include "core/settings.h"
#include "core/logging.h"
void DiscordRichPresence::ReloadSettings() {
  Settings s; s.BeginGroup("DiscordRPC");
  enabled_ = s.BoolValue("enabled", false);
}
void DiscordRichPresence::UpdatePresence(const Song &song, bool playing) {
  if (!enabled_) return;
  LogDebug("Discord: %s %s", playing ? "playing" : "paused", song.PrettyTitleWithArtist().c_str());
}
void DiscordRichPresence::Clear() {}
