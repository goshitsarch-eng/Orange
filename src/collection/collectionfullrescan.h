#ifndef STRAWBERRY_COLLECTIONFULLRESCAN_H
#define STRAWBERRY_COLLECTIONFULLRESCAN_H

#include <set>
#include <string>
#include <vector>

namespace CollectionFullRescan {

// Qt CollectionLibrary constructor: schema 21 requires a full rescan for sort tags.
inline const char *ReasonFor(int schema_version) {
  if (schema_version == 21) {
    return "Support for sort tags artist, album, album artist, title, composer and performer";
  }
  return "";
}

// Qt MainWindow::CheckFullRescanRevisions: skip new DBs and unchanged schema.
inline bool ShouldPrompt(int from, int to) { return from != 0 && from != to; }

inline std::vector<std::string> Reasons(int from, int to) {
  std::vector<std::string> reasons;
  std::set<std::string> seen;
  for (int i = from + 1; i <= to; ++i) {
    const char *reason = ReasonFor(i);
    if (reason && reason[0] && seen.insert(reason).second) {
      reasons.emplace_back(reason);
    }
  }
  return reasons;
}

inline const char *DialogTitle() { return "Collection rescan notice"; }

inline std::string DialogMessage(const std::vector<std::string> &reasons) {
  std::string message =
      "The version of Orange you've just updated to requires a full collection rescan because of the new features listed below:\n";
  for (const std::string &reason : reasons) {
    message += "• ";
    message += reason;
    message += "\n";
  }
  message += "Would you like to run a full rescan right now?";
  return message;
}

}  // namespace CollectionFullRescan

#endif
