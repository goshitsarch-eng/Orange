#ifndef STRAWBERRY_STREAMINGPROGRESS_H
#define STRAWBERRY_STREAMINGPROGRESS_H

#include <string>

namespace StreamingProgress {

constexpr int kDefaultMaximum = 100;

inline bool HasQuery(const std::string &query) { return query.find_first_not_of(" \t\n\r") != std::string::npos; }

inline bool ShouldShow(bool searching, const std::string &query) { return searching && HasQuery(query); }

inline int GetProgress(int count, int total) {
  if (total <= 0) {
    return 0;
  }
  return static_cast<int>((static_cast<float>(count) / static_cast<float>(total)) * 100.0F);
}

inline double Fraction(int value, int maximum) {
  if (maximum <= 0 || value <= 0) {
    return 0.0;
  }
  if (value >= maximum) {
    return 1.0;
  }
  return static_cast<double>(value) / static_cast<double>(maximum);
}

inline const char *Searching() { return "Searching..."; }

inline const char *ReceivingArtists() { return "Receiving artists..."; }

inline const char *ReceivingAlbums() { return "Receiving albums..."; }

inline const char *ReceivingSongs() { return "Receiving songs..."; }

inline const char *RetrievingAlbums() { return "Retrieving albums..."; }

inline std::string ReceivingAlbumsForArtists(int count) {
  if (count == 1) {
    return "Receiving albums for 1 artist...";
  }
  return "Receiving albums for " + std::to_string(count) + " artists...";
}

inline std::string ReceivingSongsForAlbums(int count) {
  if (count == 1) {
    return "Receiving songs for 1 album...";
  }
  return "Receiving songs for " + std::to_string(count) + " albums...";
}

inline std::string ReceivingCovers(int count) {
  if (count == 1) {
    return "Receiving album cover for 1 album...";
  }
  return "Receiving album covers for " + std::to_string(count) + " albums...";
}

inline std::string RetrievingSongsForAlbums(int count) {
  if (count == 1) {
    return "Retrieving songs for 1 album...";
  }
  return "Retrieving songs for " + std::to_string(count) + " albums...";
}

inline std::string RetrievingCovers(int count) {
  if (count == 1) {
    return "Retrieving album cover for 1 album...";
  }
  return "Retrieving album covers for " + std::to_string(count) + " albums...";
}

}  // namespace StreamingProgress

#endif
