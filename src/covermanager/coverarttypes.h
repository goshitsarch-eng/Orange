#ifndef STRAWBERRY_COVERARTTYPES_H
#define STRAWBERRY_COVERARTTYPES_H

#include "covermanager/coveroptions.h"
#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace CoverArtTypes {

struct Entry {
  std::string id;
  bool enabled = true;
};

inline const std::vector<std::string> &AllIds() {
  static const std::vector<std::string> kAll = {"art_unset", "art_manual", "art_automatic", "art_embedded"};
  return kAll;
}

inline const std::vector<std::string> &DefaultEnabledIds() {
  static const std::vector<std::string> kDefault = {"art_embedded", "art_automatic", "art_manual"};
  return kDefault;
}

inline std::string DefaultSaved() { return StrUtils::Join(DefaultEnabledIds(), ","); }

inline bool IsKnown(const std::string &id) {
  return id == "art_unset" || id == "art_manual" || id == "art_automatic" || id == "art_embedded";
}

inline std::string Description(const std::string &id) {
  if (id == "art_unset") {
    return "Manually unset (" + id + ")";
  }
  if (id == "art_manual") {
    return "Set through album cover search (" + id + ")";
  }
  if (id == "art_automatic") {
    return "Automatically picked up from album directory (" + id + ")";
  }
  if (id == "art_embedded") {
    return "Embedded album cover art (" + id + ")";
  }
  return id;
}

inline bool ContainsId(const std::vector<Entry> &entries, const std::string &id) {
  for (const Entry &entry : entries) {
    if (entry.id == id) {
      return true;
    }
  }
  return false;
}

inline std::vector<Entry> Parse(const std::string &saved) {
  std::vector<Entry> entries;
  for (const std::string &part : StrUtils::Split(saved, ',')) {
    const std::string id = StrUtils::Trim(part);
    if (!IsKnown(id) || ContainsId(entries, id)) {
      continue;
    }
    entries.push_back({id, true});
  }
  for (const std::string &id : AllIds()) {
    if (!ContainsId(entries, id)) {
      entries.push_back({id, false});
    }
  }
  return entries;
}

inline std::vector<std::string> EnabledIds(const std::vector<Entry> &entries) {
  std::vector<std::string> ids;
  for (const Entry &entry : entries) {
    if (entry.enabled) {
      ids.push_back(entry.id);
    }
  }
  return ids;
}

inline std::string Save(const std::vector<Entry> &entries) { return StrUtils::Join(EnabledIds(entries), ","); }

inline std::vector<Entry> Move(std::vector<Entry> entries, int index, int delta) {
  const int dest = index + delta;
  if (index < 0 || dest < 0 || dest >= static_cast<int>(entries.size())) {
    return entries;
  }
  std::swap(entries[static_cast<size_t>(index)], entries[static_cast<size_t>(dest)]);
  return entries;
}

inline bool FilenameGroupEnabled(CoverOptions::CoverType save_type) { return save_type == CoverOptions::CoverType::Album; }

inline bool FilenameGroupEnabled(const std::string &save_type) { return FilenameGroupEnabled(CoverOptions::TypeFromValue(save_type)); }

inline bool FilenamePatternOptionsEnabled(CoverOptions::CoverType save_type, CoverOptions::CoverFilename filename) {
  return FilenameGroupEnabled(save_type) && filename == CoverOptions::CoverFilename::Pattern;
}

inline bool FilenamePatternOptionsEnabled(const std::string &save_type, const std::string &filename) {
  return FilenamePatternOptionsEnabled(CoverOptions::TypeFromValue(save_type), CoverOptions::FilenameModeFromValue(filename));
}

}  // namespace CoverArtTypes

#endif
