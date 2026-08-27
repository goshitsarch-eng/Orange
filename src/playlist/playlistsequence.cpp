#include "playlist/playlistsequence.h"

#include "core/settings.h"
#include "translations/translations.h"

PlaylistSequence::PlaylistSequence() { Load(); }

void PlaylistSequence::Load() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  repeat_mode_ = static_cast<RepeatMode>(settings.IntValue("repeat_mode", static_cast<int>(RepeatMode::Off)));
  shuffle_mode_ = static_cast<ShuffleMode>(settings.IntValue("shuffle_mode", static_cast<int>(ShuffleMode::Off)));
}

void PlaylistSequence::Save() const {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetIntValue("repeat_mode", static_cast<int>(repeat_mode_));
  settings.SetIntValue("shuffle_mode", static_cast<int>(shuffle_mode_));
}

void PlaylistSequence::SetRepeatMode(RepeatMode mode) {
  repeat_mode_ = mode;
  Save();
}

void PlaylistSequence::SetShuffleMode(ShuffleMode mode) {
  shuffle_mode_ = mode;
  Save();
}

void PlaylistSequence::CycleRepeatMode() {
  switch (repeat_mode_) {
    case RepeatMode::Off:
      SetRepeatMode(RepeatMode::Track);
      break;
    case RepeatMode::Track:
      SetRepeatMode(RepeatMode::Album);
      break;
    case RepeatMode::Album:
      SetRepeatMode(RepeatMode::Playlist);
      break;
    case RepeatMode::Playlist:
      SetRepeatMode(RepeatMode::OneByOne);
      break;
    case RepeatMode::OneByOne:
      SetRepeatMode(RepeatMode::Intro);
      break;
    case RepeatMode::Intro:
    default:
      SetRepeatMode(RepeatMode::Off);
      break;
  }
}

void PlaylistSequence::CycleShuffleMode() {
  switch (shuffle_mode_) {
    case ShuffleMode::Off:
      SetShuffleMode(ShuffleMode::All);
      break;
    case ShuffleMode::All:
      SetShuffleMode(ShuffleMode::InsideAlbum);
      break;
    case ShuffleMode::InsideAlbum:
      SetShuffleMode(ShuffleMode::Albums);
      break;
    case ShuffleMode::Albums:
      SetShuffleMode(ShuffleMode::Grouping);
      break;
    case ShuffleMode::Grouping:
    default:
      SetShuffleMode(ShuffleMode::Off);
      break;
  }
}

const char *PlaylistSequence::RepeatLabel(RepeatMode mode) {
  switch (mode) {
    case RepeatMode::Track:
      return Translations::CStr("Repeat track");
    case RepeatMode::Album:
      return Translations::CStr("Repeat album");
    case RepeatMode::Playlist:
      return Translations::CStr("Repeat playlist");
    case RepeatMode::OneByOne:
      return Translations::CStr("Stop after each track");
    case RepeatMode::Intro:
      return Translations::CStr("Intro tracks");
    case RepeatMode::Off:
    default:
      return Translations::CStr("Don't repeat");
  }
}

const char *PlaylistSequence::ShuffleLabel(ShuffleMode mode) {
  switch (mode) {
    case ShuffleMode::All:
      return Translations::CStr("Shuffle all");
    case ShuffleMode::InsideAlbum:
      return Translations::CStr("Shuffle tracks in this album");
    case ShuffleMode::Albums:
      return Translations::CStr("Shuffle albums");
    case ShuffleMode::Grouping:
      return Translations::CStr("Shuffle grouping");
    case ShuffleMode::Off:
    default:
      return Translations::CStr("Don't shuffle");
  }
}
