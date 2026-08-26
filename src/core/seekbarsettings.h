#ifndef STRAWBERRY_SEEKBARSETTINGS_H
#define STRAWBERRY_SEEKBARSETTINGS_H

namespace SeekbarSettings {

constexpr char kSettingsGroup[] = "Seekbar";
constexpr char kShowRemaining[] = "show_remaining";

enum class Mode {
  Normal = 0,
  Moodbar = 1,
  Waveform = 2
};

constexpr char kMode[] = "mode";
constexpr Mode kDefaultMode = Mode::Normal;

}  // namespace SeekbarSettings

#endif
