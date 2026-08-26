#ifndef STRAWBERRY_STANDARDPATHS_H
#define STRAWBERRY_STANDARDPATHS_H

#include <string>

namespace StandardPaths {

std::string ConfigDir();
std::string DataDir();
std::string CacheDir();
std::string DatabasePath();
std::string SettingsPath();
std::string CoverCacheDir();
std::string LyricsCacheDir();
std::string MoodbarCacheDir();
std::string WaveformCacheDir();

}  // namespace StandardPaths

#endif
