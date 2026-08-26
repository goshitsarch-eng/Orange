#ifndef STRAWBERRY_RADIOTREE_H
#define STRAWBERRY_RADIOTREE_H

#include "radios/radiochannel.h"
#include "radios/radiomenu.h"

#include <algorithm>
#include <set>
#include <vector>

namespace RadioTree {

enum class Kind { Service, Channel };

struct Row {
  Kind kind = Kind::Channel;
  Song::Source source = Song::Source::Unknown;
  RadioChannel channel;
  int child_count = 0;
};

inline std::vector<Song::Source> ServiceOrder() {
  return {Song::Source::SomaFM, Song::Source::RadioParadise, Song::Source::RadioBrowser, Song::Source::Stream};
}

inline bool ShowChildren(Song::Source source, const std::set<Song::Source> &collapsed) {
  return collapsed.find(source) == collapsed.end();
}

inline bool Toggle(std::set<Song::Source> *collapsed, Song::Source source) {
  if (!collapsed) {
    return false;
  }
  if (collapsed->erase(source) == 0) {
    collapsed->insert(source);
    return false;
  }
  return true;
}

inline std::vector<RadioChannel> ChannelsForSource(const std::vector<RadioChannel> &channels, Song::Source source) {
  std::vector<RadioChannel> matches;
  for (const RadioChannel &channel : channels) {
    if (channel.source == source) {
      matches.push_back(channel);
    }
  }
  return matches;
}

inline std::vector<Song::Source> PresentSources(const std::vector<RadioChannel> &channels) {
  std::vector<Song::Source> sources;
  for (Song::Source source : ServiceOrder()) {
    if (!ChannelsForSource(channels, source).empty()) {
      sources.push_back(source);
    }
  }
  for (const RadioChannel &channel : channels) {
    if (std::find(sources.begin(), sources.end(), channel.source) == sources.end()) {
      sources.push_back(channel.source);
    }
  }
  return sources;
}

inline std::vector<Row> VisibleRows(const std::vector<RadioChannel> &channels, const std::set<Song::Source> &collapsed) {
  std::vector<Row> rows;
  for (Song::Source source : PresentSources(channels)) {
    const std::vector<RadioChannel> children = ChannelsForSource(channels, source);
    Row service;
    service.kind = Kind::Service;
    service.source = source;
    service.child_count = static_cast<int>(children.size());
    rows.push_back(service);
    if (!ShowChildren(source, collapsed)) {
      continue;
    }
    for (const RadioChannel &channel : children) {
      Row row;
      row.kind = Kind::Channel;
      row.source = source;
      row.channel = channel;
      rows.push_back(row);
    }
  }
  return rows;
}

inline bool ActivateExpands(Kind kind) { return kind == Kind::Service; }

inline bool ActivatePlays(Kind kind) { return kind == Kind::Channel; }

inline std::string ServiceLabel(Song::Source source, int child_count, bool expanded) {
  std::string label = expanded ? "▼ " : "▶ ";
  label += RadioMenu::ServiceName(source);
  if (child_count > 0) {
    label += " (" + std::to_string(child_count) + ")";
  }
  return label;
}

}  // namespace RadioTree

#endif
