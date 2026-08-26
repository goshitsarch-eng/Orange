#ifndef STRAWBERRY_ALBUMCOVEREXPORT_H
#define STRAWBERRY_ALBUMCOVEREXPORT_H

#include <string>

class AlbumCoverExport {
 public:
  enum class OverwriteMode {
    None = 0,
    All = 1,
    Smaller = 2
  };

  struct DialogResult {
    bool cancelled = false;
    bool export_downloaded = false;
    bool export_embedded = false;
    std::string filename = "cover";
    OverwriteMode overwrite = OverwriteMode::None;
    bool forcesize = false;
    int width = 0;
    int height = 0;

    bool IsSizeForced() const { return forcesize && width > 0 && height > 0; }
    bool RequiresCoverProcessing() const { return IsSizeForced() || overwrite == OverwriteMode::Smaller; }
  };
};

#endif
