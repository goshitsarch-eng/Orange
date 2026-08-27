#ifndef STRAWBERRY_ALBUMCOVERSEARCHER_H
#define STRAWBERRY_ALBUMCOVERSEARCHER_H

#include "covermanager/albumcoverfetcher.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/albumcoversearcherlabels.h"

#include "core/song.h"
#include "widgets/listboxkeyboard.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <functional>
#include <string>

class Application;

class AlbumCoverSearcher {
 public:
  static constexpr int kIconSize = 120;
  static constexpr int kMinColumns = 2;
  static constexpr int kMaxColumns = 6;
  static constexpr int kCellPadding = 24;

  static void Show(GtkWindow *parent, Application *app);
  static void Show(GtkWindow *parent, Application *app, const Song &song, std::function<void(bool)> done = {});

  static Song PreferredSong(const Song &requested, const Song &fallback) { return requested.is_valid() ? requested : fallback; }

  static int ColumnsForWidth(int width) {
    if (width <= 0) {
      return 3;
    }
    return std::clamp(width / (kIconSize + kCellPadding), kMinColumns, kMaxColumns);
  }

  static std::string DimensionLabel(int width, int height) {
    if (width <= 0 || height <= 0) {
      return {};
    }
    return std::to_string(width) + "×" + std::to_string(height);
  }

  static std::string CellSubtitle(const CoverProviderSearchResult &result) {
    std::string text = result.provider;
    const std::string size = DimensionLabel(result.image_width, result.image_height);
    if (!size.empty()) {
      text += " · " + size;
    }
    return text;
  }

  static bool CanLoadThumb(const CoverProviderSearchResult &result) {
    return !result.image_data.empty() || AlbumCoverFetcherSearch::IsHttpUrl(result.image_url);
  }

  // Qt AlbumCoverSearcher::Search: album enabled means idle; click starts. Disabled means in-flight; click aborts.
  static const char *SearchButtonLabel(bool searching) { return searching ? AlbumCoverSearcherLabels::Abort() : AlbumCoverSearcherLabels::Search(); }

  static bool FieldsEnabled(bool searching) { return !searching; }

  static bool GridEnabled(bool searching) { return !searching; }

  static bool BusyVisible(bool searching) { return searching; }

  static bool ShouldStartSearch(bool searching) { return !searching; }

  static bool ShouldAbortSearch(bool searching) { return searching; }

  static bool ShouldAutoSearch(const std::string &artist, const std::string &album) { return !artist.empty() || !album.empty(); }

  // Qt AlbumCoverSearcher::keyPressEvent ignores Enter so the default OK does not accept.
  static bool ShouldIgnoreEnter(unsigned keyval) {
    return keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter;
  }
};

#endif
