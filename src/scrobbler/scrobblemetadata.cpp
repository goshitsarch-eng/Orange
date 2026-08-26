#include "scrobbler/scrobblemetadata.h"

#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "utilities/strutils.h"

std::string ScrobbleMetadata::StripRemasteredTitle(const std::string &title) {
  std::string result = title;
  for (int pass = 0; pass < 3; ++pass) {
    if (result.size() < 3) {
      break;
    }
    const char close = result.back();
    const char open = close == ')' ? '(' : close == ']' ? '[' : '\0';
    if (!open) {
      break;
    }
    const auto pos = result.find_last_of(open);
    if (pos == std::string::npos) {
      break;
    }
    const std::string inner = result.substr(pos);
    if (!StrUtils::ContainsInsensitive(inner, "remaster") && !StrUtils::ContainsInsensitive(inner, "remix") &&
        !StrUtils::ContainsInsensitive(inner, "edition") && !StrUtils::ContainsInsensitive(inner, "anniversary")) {
      break;
    }
    size_t end = pos;
    while (end > 0 && (result[end - 1] == ' ' || result[end - 1] == '\t')) {
      --end;
    }
    result.resize(end);
  }
  return result;
}

ScrobbleMetadata ScrobbleMetadata::FromSong(const Song &song, uint64_t timestamp, bool prefer_album_artist, bool strip_remastered) {
  ScrobbleMetadata metadata;
  metadata.artist = prefer_album_artist ? song.EffectiveAlbumartist() : song.artist();
  if (metadata.artist.empty()) {
    metadata.artist = song.artist();
  }
  metadata.album = song.album();
  metadata.title = strip_remastered ? StripRemasteredTitle(song.title()) : song.title();
  metadata.albumartist = song.EffectiveAlbumartist();
  metadata.track = song.track();
  metadata.length_nanosec = song.length_nanosec();
  metadata.timestamp = timestamp;
  return metadata;
}

ScrobbleMetadata ScrobbleMetadata::FromSongSettings(const Song &song, uint64_t timestamp) {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  return FromSong(song, timestamp, settings.BoolValue(ScrobblerSettings::kAlbumArtist, ScrobblerSettings::kDefaultAlbumArtist),
                  settings.BoolValue(ScrobblerSettings::kStripRemastered, ScrobblerSettings::kDefaultStripRemastered));
}
