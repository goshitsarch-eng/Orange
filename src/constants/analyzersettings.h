#ifndef ANALYZERSETTINGS_H
#define ANALYZERSETTINGS_H

namespace AnalyzerSettings {

constexpr char kSettingsGroup[] = "Analyzer";
constexpr char kType[] = "type";
constexpr char kEnabled[] = "enabled";
constexpr char kFramerate[] = "framerate";

constexpr char kDefaultType[] = "Bar";
constexpr bool kDefaultEnabled = true;
constexpr int kLowFramerate = 20;
constexpr int kMediumFramerate = 25;
constexpr int kHighFramerate = 30;
constexpr int kSuperHighFramerate = 60;
constexpr int kDefaultFramerate = kMediumFramerate;
constexpr int kMinFramerate = 5;
constexpr int kMaxFramerate = kSuperHighFramerate;

}  // namespace AnalyzerSettings

#endif
