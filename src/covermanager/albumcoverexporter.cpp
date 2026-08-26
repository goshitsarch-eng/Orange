#include "covermanager/albumcoverexporter.h"

#include "covermanager/coverexportrunnable.h"

AlbumCoverExporter::AlbumCoverExporter(TagReader *tagreader) : tagreader_(tagreader) {
  cover_types_ = AlbumCoverLoaderOptions::LoadTypes();
}

void AlbumCoverExporter::SetDialogResult(const AlbumCoverExport::DialogResult &dialog_result) { dialog_result_ = dialog_result; }

void AlbumCoverExporter::SetCoverTypes(const AlbumCoverLoaderOptions::Types &cover_types) { cover_types_ = cover_types; }

void AlbumCoverExporter::AddExportRequest(const Song &song) { requests_.push_back(song); }

void AlbumCoverExporter::Cancel() { requests_.clear(); }

void AlbumCoverExporter::StartExporting() {
  exported_ = 0;
  skipped_ = 0;
  for (const Song &song : requests_) {
    CoverExportRunnable job(tagreader_, dialog_result_, cover_types_, song);
    if (job.Run()) {
      ++exported_;
    } else {
      ++skipped_;
    }
  }
}
