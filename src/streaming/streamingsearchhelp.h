#ifndef STREAMING_STREAMINGSEARCHHELP_H_
#define STREAMING_STREAMINGSEARCHHELP_H_

namespace StreamingSearchHelp {

inline const char *IdleText() { return "Enter search terms above to find music"; }
inline const char *Artists() { return "artists"; }
inline const char *Albums() { return "albums"; }
inline const char *Songs() { return "songs"; }

inline const char *EmptyResultsText() { return "No results"; }

inline const char *LabelFor(const bool has_searched) {
  return has_searched ? EmptyResultsText() : IdleText();
}

}  // namespace StreamingSearchHelp

#endif  // STREAMING_STREAMINGSEARCHHELP_H_
