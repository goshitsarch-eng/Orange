#ifndef STRAWBERRY_SMARTPLAYLISTSUMMARY_H
#define STRAWBERRY_SMARTPLAYLISTSUMMARY_H

#include "smartplaylists/smartplaylist.h"

#include <string>

namespace SmartPlaylistSummary {

inline const char *Title() { return "Dynamic mode is on"; }
inline const char *EmptyTerms() { return "New tracks will be added automatically."; }

inline std::string FieldName(SmartPlaylistField field) {
  const std::vector<std::string> names = SmartPlaylistSearch::FieldNames();
  const int index = static_cast<int>(field);
  if (index < 0 || static_cast<size_t>(index) >= names.size()) {
    return "Title";
  }
  return names[static_cast<size_t>(index)];
}

inline std::string TermText(const SmartPlaylistTerm &term) {
  std::string text = FieldName(term.field) + " " + SmartPlaylistSearch::OpName(term.op);
  if (term.op != SmartPlaylistOp::Empty && term.op != SmartPlaylistOp::NotEmpty && !term.value.empty()) {
    text += " " + term.value;
  }
  return text;
}

inline std::string LimitSuffix(const SmartPlaylistSearch &search) {
  if (search.limit > 0) {
    return " · limit " + std::to_string(search.limit);
  }
  return {};
}

inline std::string Summary(const SmartPlaylistSearch &search) {
  if (search.type == SmartPlaylistSearch::SearchType::All) {
    return std::string("Include all songs") + LimitSuffix(search);
  }
  if (search.terms.empty()) {
    return EmptyTerms();
  }
  std::string out;
  const char *join = search.type == SmartPlaylistSearch::SearchType::Or ? " or " : " and ";
  for (size_t i = 0; i < search.terms.size(); ++i) {
    if (i > 0) {
      out += join;
    }
    out += TermText(search.terms[i]);
  }
  out += LimitSuffix(search);
  return out;
}

inline std::string FinishText(int count, const std::string &name, const SmartPlaylistSearch &search) {
  return std::to_string(count) + " songs will be added as “" + name + "”. " + Summary(search);
}

}  // namespace SmartPlaylistSummary

#endif
