#ifndef STRAWBERRY_ALBUMCOVEREXPORTER_H
#define STRAWBERRY_ALBUMCOVEREXPORTER_H

#include "core/song.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverloaderoptions.h"

#include <vector>

class TagReader;

class AlbumCoverExporter {
 public:
  explicit AlbumCoverExporter(TagReader *tagreader);

  void SetDialogResult(const AlbumCoverExport::DialogResult &dialog_result);
  void SetCoverTypes(const AlbumCoverLoaderOptions::Types &cover_types);
  void AddExportRequest(const Song &song);
  void StartExporting();
  void Cancel();

  int request_count() const { return static_cast<int>(requests_.size()); }
  int exported() const { return exported_; }
  int skipped() const { return skipped_; }

 private:
  TagReader *tagreader_;
  AlbumCoverLoaderOptions::Types cover_types_;
  AlbumCoverExport::DialogResult dialog_result_;
  std::vector<Song> requests_;
  int exported_ = 0;
  int skipped_ = 0;
};

#endif
