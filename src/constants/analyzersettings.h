#ifndef ANALYZERSETTINGS_H
#define ANALYZERSETTINGS_H

namespace AnalyzerSettings {

constexpr char kSettingsGroup[] = "Analyzer";
constexpr char kType[] = "type";
constexpr char kEnabled[] = "enabled";
constexpr char kFramerate[] = "framerate";

constexpr char kDefaultType[] = "Bar";
constexpr bool kDefaultEnabled = true;
constexpr int kDefaultFramerate = 25;
constexpr int kMinFramerate = 5;
constexpr int kMaxFramerate = 60;

}  // namespace AnalyzerSettings

#endif
