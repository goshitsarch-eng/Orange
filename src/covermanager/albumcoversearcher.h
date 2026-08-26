#ifndef STRAWBERRY_ALBUMCOVERSEARCHER_H
#define STRAWBERRY_ALBUMCOVERSEARCHER_H

#include "covermanager/albumcoverfetcher.h"
#include "covermanager/albumcoverfetchersearch.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <string>

class Application;

class AlbumCoverSearcher {
 public:
  static constexpr int kIconSize = 120;
  static constexpr int kMinColumns = 2;
  static constexpr int kMaxColumns = 6;
  static constexpr int kCellPadding = 24;

  static void Show(GtkWindow *parent, Application *app);

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
};

#endif
