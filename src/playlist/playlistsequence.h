#ifndef STRAWBERRY_PLAYLISTSEQUENCE_H
#define STRAWBERRY_PLAYLISTSEQUENCE_H

#include <string>

class PlaylistSequence {
 public:
  enum class RepeatMode {
    Off = 0,
    Track = 1,
    Album = 2,
    Playlist = 3,
    OneByOne = 4,
    Intro = 5
  };
  enum class ShuffleMode {
    Off = 0,
    All = 1,
    InsideAlbum = 2,
    Albums = 3,
    Grouping = 4
  };

  static constexpr const char *kSettingsGroup = "PlaylistSequence";

  PlaylistSequence();

  RepeatMode repeat_mode() const { return repeat_mode_; }
  ShuffleMode shuffle_mode() const { return shuffle_mode_; }

  void SetRepeatMode(RepeatMode mode);
  void SetShuffleMode(ShuffleMode mode);
  void CycleRepeatMode();
  void CycleShuffleMode();
  void Load();
  void Save() const;

  static const char *RepeatLabel(RepeatMode mode);
  static const char *ShuffleLabel(ShuffleMode mode);

 private:
  RepeatMode repeat_mode_ = RepeatMode::Off;
  ShuffleMode shuffle_mode_ = ShuffleMode::Off;
};

#endif
