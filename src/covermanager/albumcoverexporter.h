#ifndef STRAWBERRY_ALBUMCOVEREXPORTER_H
#define STRAWBERRY_ALBUMCOVEREXPORTER_H

#include "core/signal.h"
#include "core/song.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverloaderoptions.h"

#include <vector>

class TagReader;

class AlbumCoverExporter {
 public:
  explicit AlbumCoverExporter(TagReader *tagreader);
  ~AlbumCoverExporter();

  void SetDialogResult(const AlbumCoverExport::DialogResult &dialog_result);
  void SetCoverTypes(const AlbumCoverLoaderOptions::Types &cover_types);
  void AddExportRequest(const Song &song);
  void StartExporting();
  void StartExportingAsync();
  void ProcessSome();
  void Cancel();
  void IdleTick();

  int request_count() const { return static_cast<int>(requests_.size()); }
  int exported() const { return exported_; }
  int skipped() const { return skipped_; }
  int next_index() const { return next_; }
  bool finished() const { return finished_; }

  Signal<> ExportUpdate;
  Signal<> Finished;

 private:
  void Complete();
  void ScheduleIdle();

  TagReader *tagreader_;
  AlbumCoverLoaderOptions::Types cover_types_;
  AlbumCoverExport::DialogResult dialog_result_;
  std::vector<Song> requests_;
  int exported_ = 0;
  int skipped_ = 0;
  int next_ = 0;
  bool cancelled_ = false;
  bool finished_ = false;
  bool async_ = false;
  unsigned idle_id_ = 0;
};

#endif
