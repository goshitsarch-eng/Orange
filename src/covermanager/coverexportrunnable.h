#ifndef STRAWBERRY_COVEREXPORTRUNNABLE_H
#define STRAWBERRY_COVEREXPORTRUNNABLE_H

#include "core/song.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverloaderoptions.h"

#include <string>
#include <vector>

class TagReader;

class CoverExportRunnable {
 public:
  CoverExportRunnable(TagReader *tagreader, const AlbumCoverExport::DialogResult &dialog_result,
                      const AlbumCoverLoaderOptions::Types &cover_types, const Song &song);

  bool Run();
  static std::string DestinationPath(const Song &song, const AlbumCoverExport::DialogResult &dialog_result, const std::string &extension);

 private:
  bool ExportCover();
  bool ProcessAndExportCover();
  bool LoadSource(std::string *path, std::string *extension, std::vector<unsigned char> *embedded);

  TagReader *tagreader_;
  AlbumCoverExport::DialogResult dialog_result_;
  AlbumCoverLoaderOptions::Types cover_types_;
  Song song_;
};

#endif
