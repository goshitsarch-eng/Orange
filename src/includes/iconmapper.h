#ifndef STRAWBERRY_ICONMAPPER_H
#define STRAWBERRY_ICONMAPPER_H

#include <string>

inline std::string MapIconName(const std::string &name) { return name.empty() ? "audio-x-generic" : name; }

#endif
