#include "covermanager/albumcoverexporter.h"

#include "covermanager/coverexportjob.h"
#include "covermanager/coverexportrunnable.h"

#include <glib.h>

namespace {

gboolean CoverExportProcessIdle(gpointer data) {
  static_cast<AlbumCoverExporter *>(data)->IdleTick();
  return G_SOURCE_REMOVE;
}

}  // namespace

AlbumCoverExporter::AlbumCoverExporter(TagReader *tagreader) : tagreader_(tagreader) {
  cover_types_ = AlbumCoverLoaderOptions::LoadTypes();
}

AlbumCoverExporter::~AlbumCoverExporter() {
  if (idle_id_) {
    g_source_remove(idle_id_);
    idle_id_ = 0;
  }
}

void AlbumCoverExporter::SetDialogResult(const AlbumCoverExport::DialogResult &dialog_result) { dialog_result_ = dialog_result; }

void AlbumCoverExporter::SetCoverTypes(const AlbumCoverLoaderOptions::Types &cover_types) { cover_types_ = cover_types; }

void AlbumCoverExporter::AddExportRequest(const Song &song) { requests_.push_back(song); }

void AlbumCoverExporter::Cancel() { cancelled_ = true; }

void AlbumCoverExporter::StartExporting() {
  exported_ = 0;
  skipped_ = 0;
  next_ = 0;
  cancelled_ = false;
  finished_ = false;
  async_ = false;
  while (!finished_) {
    ProcessSome();
  }
}

void AlbumCoverExporter::StartExportingAsync() {
  exported_ = 0;
  skipped_ = 0;
  next_ = 0;
  cancelled_ = false;
  finished_ = false;
  async_ = true;
  ScheduleIdle();
}

void AlbumCoverExporter::IdleTick() {
  idle_id_ = 0;
  ProcessSome();
}

void AlbumCoverExporter::ScheduleIdle() {
  if (idle_id_) {
    return;
  }
  idle_id_ = g_idle_add(CoverExportProcessIdle, this);
}

void AlbumCoverExporter::Complete() {
  finished_ = true;
  ExportUpdate.Emit();
  Finished.Emit();
}

void AlbumCoverExporter::ProcessSome() {
  if (finished_) {
    return;
  }
  const int total = static_cast<int>(requests_.size());
  if (!CoverExportJob::ShouldProcessBatch(cancelled_)) {
    if (CoverExportJob::ShouldFinish(next_, total, cancelled_)) {
      Complete();
    }
    return;
  }
  int processed = 0;
  while (processed < CoverExportJob::kMaxConcurrent && next_ < total) {
    CoverExportRunnable job(tagreader_, dialog_result_, cover_types_, requests_[static_cast<size_t>(next_)]);
    if (job.Run()) {
      ++exported_;
    } else {
      ++skipped_;
    }
    ++next_;
    ++processed;
  }
  ExportUpdate.Emit();
  if (CoverExportJob::ShouldFinish(next_, total, cancelled_)) {
    Complete();
    return;
  }
  if (CoverExportJob::ShouldScheduleNext(next_, total, cancelled_, async_)) {
    ScheduleIdle();
  }
}
