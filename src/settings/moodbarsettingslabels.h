#ifndef STRAWBERRY_MOODBARSETTINGSLABELS_H
#define STRAWBERRY_MOODBARSETTINGSLABELS_H

#include "constants/moodbarsettings.h"
#include "moodbar/moodbarstyle.h"

#include <string>
#include <utility>
#include <vector>

namespace MoodbarSettingsLabels {

inline const char *StyleLabel() { return "Moodbar style"; }
inline const char *SaveLabel() { return "Save the .mood files directly in the songs folders"; }

inline std::vector<std::pair<std::string, std::string>> StyleChoices() {
  std::vector<std::pair<std::string, std::string>> choices;
  for (int i = 0; i < static_cast<int>(MoodbarSettings::Style::StyleCount); ++i) {
    choices.emplace_back(std::to_string(i), MoodbarStyle::StyleName(static_cast<MoodbarSettings::Style>(i)));
  }
  return choices;
}

}  // namespace MoodbarSettingsLabels

#endif  // STRAWBERRY_MOODBARSETTINGSLABELS_H
