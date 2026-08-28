#ifndef STRAWBERRY_RADIOVIEWSEARCH_H
#define STRAWBERRY_RADIOVIEWSEARCH_H

namespace RadioViewSearch {

inline const char *BrowserTabId() { return "browser"; }

inline const char *ChannelsTabId() { return "channels"; }

inline bool ShouldShowBrowserTab() { return true; }

inline bool ShouldRunSearch() { return true; }

inline bool ShouldFocusSearch() { return true; }

}  // namespace RadioViewSearch

#endif
