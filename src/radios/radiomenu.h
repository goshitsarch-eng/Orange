#ifndef STRAWBERRY_RADIOMENU_H
#define STRAWBERRY_RADIOMENU_H

#include "radios/radiochannel.h"
#include "radios/radiobrowserservice.h"
#include "radios/radioparadiseservice.h"
#include "radios/somafmservice.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace RadioMenu {

enum class Action { Append, Replace, New, Queue, Homepage, Donate, Refresh };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::Append;
};

inline std::vector<Item> Items() {
  return {
      {"Append to current playlist", "append", Action::Append},
      {"Replace current playlist", "replace", Action::Replace},
      {"Open in new playlist", "new", Action::New},
      {"Queue track", "enqueue", Action::Queue},
      {"Open homepage", "homepage", Action::Homepage},
      {"Donate", "donate", Action::Donate},
      {"Refresh channels", "refresh", Action::Refresh},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

// Qt RadioView::contextMenuEvent always pops from Menu / Shift+F10.
constexpr unsigned kMenuKey = 0xff67;
constexpr unsigned kF10Key = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenuKey || (keyval == kF10Key && (state & kShiftMask) != 0);
}

inline bool ShouldShowMenu() { return true; }

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Append;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Append;
}

inline bool NeedsSelection(Action action) { return action != Action::Refresh; }

inline std::vector<Item> VisibleItems(bool has_selection) {
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (!NeedsSelection(item.action) || has_selection) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline bool Contains(const std::vector<Item> &items, Action action) {
  for (const Item &item : items) {
    if (item.action == action) {
      return true;
    }
  }
  return false;
}

inline const char *WinAction(Action action) {
  switch (action) {
    case Action::Append:
      return "win.radio-append";
    case Action::Replace:
      return "win.radio-replace";
    case Action::New:
      return "win.radio-new";
    case Action::Queue:
      return "win.radio-enqueue";
    case Action::Homepage:
      return "win.radio-homepage";
    case Action::Donate:
      return "win.radio-donate";
    case Action::Refresh:
      return "win.radio-refresh";
  }
  return "win.radio-append";
}

inline const char *ServiceName(Song::Source source) {
  switch (source) {
    case Song::Source::SomaFM:
      return "SomaFM";
    case Song::Source::RadioParadise:
      return "Radio Paradise";
    case Song::Source::RadioBrowser:
      return "Radio Browser";
    case Song::Source::Stream:
      return "Stream";
    default:
      return "Unknown";
  }
}

inline std::string HomepageUrl(Song::Source source) {
  switch (source) {
    case Song::Source::SomaFM:
      return SomaFMService::Homepage();
    case Song::Source::RadioParadise:
      return RadioParadiseService::Homepage();
    case Song::Source::RadioBrowser:
      return RadioBrowserService::Homepage();
    default:
      return {};
  }
}

inline std::string DonateUrl(Song::Source source) {
  switch (source) {
    case Song::Source::SomaFM:
      return SomaFMService::Donate();
    case Song::Source::RadioParadise:
      return RadioParadiseService::Donate();
    case Song::Source::RadioBrowser:
      return RadioBrowserService::Donate();
    default:
      return {};
  }
}

inline std::vector<std::string> UniqueUrls(const std::vector<RadioChannel> &channels, bool donate) {
  std::vector<std::string> urls;
  for (const RadioChannel &channel : channels) {
    const std::string url = donate ? DonateUrl(channel.source) : HomepageUrl(channel.source);
    if (url.empty()) {
      continue;
    }
    if (std::find(urls.begin(), urls.end(), url) == urls.end()) {
      urls.push_back(url);
    }
  }
  return urls;
}

}  // namespace RadioMenu

#endif
