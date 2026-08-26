#ifndef STRAWBERRY_CONTEXTIDLE_H
#define STRAWBERRY_CONTEXTIDLE_H

#include "context/contexttechnical.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace ContextIdle {

inline const char *Headline() { return "No song playing"; }

inline double FontScale() { return 1.6; }

inline int IdleFontSizePt(double headline_pt) { return std::max(1, static_cast<int>(std::lround(headline_pt * FontScale()))); }

inline std::string TotalsMarkup(int songs, int artists, int albums) { return ContextTechnical::Totals(songs, artists, albums); }

}  // namespace ContextIdle

#endif  // STRAWBERRY_CONTEXTIDLE_H
