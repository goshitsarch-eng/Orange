#ifndef STRAWBERRY_GSTENGINESOURCESETUP_H
#define STRAWBERRY_GSTENGINESOURCESETUP_H

#include "version.h"

#include <string>

namespace GstSourceSetup {

// Qt GstEnginePipeline::SourceSetupCallback: "Strawberry {version}".
inline std::string UserAgentString() { return std::string("Strawberry ") + STRAWBERRY_VERSION_DISPLAY; }

inline bool ShouldSetDevice(const std::string &device) { return !device.empty(); }

// Qt always sets automatic-redirect TRUE on HTTP sources.
inline bool AutomaticRedirect() { return true; }

}  // namespace GstSourceSetup

#endif  // STRAWBERRY_GSTENGINESOURCESETUP_H
