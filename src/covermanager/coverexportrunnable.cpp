#include "covermanager/coverexportrunnable.h"

#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

CoverExportRunnable::CoverExportRunnable(TagReader *tagreader, const AlbumCoverExport::DialogResult &dialog_result,
                                         const AlbumCoverLoaderOptions::Types &cover_types, const Song &song)
    : tagreader_(tagreader), dialog_result_(dialog_result), cover_types_(cover_types), song_(song) {}

std::string CoverExportRunnable::DestinationPath(const Song &song, const AlbumCoverExport::DialogResult &dialog_result, const std::string &extension) {
  const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
  std::string filename = dialog_result.filename.empty() ? "cover" : dialog_result.filename;
  if (!extension.empty()) {
    filename += "." + extension;
  }
  return FileUtils::Join(dir.empty() ? "." : dir, filename);
}

bool CoverExportRunnable::LoadSource(std::string *path, std::string *extension, std::vector<unsigned char> *embedded) {
  for (const AlbumCoverLoaderOptions::Type type : cover_types_) {
    switch (type) {
      case AlbumCoverLoaderOptions::Type::Unset:
        if (song_.art_unset()) {
          return false;
        }
        break;
      case AlbumCoverLoaderOptions::Type::Embedded:
        if (song_.art_embedded() && dialog_result_.export_embedded && tagreader_) {
          auto cover = tagreader_->LoadCoverData(FileUtils::PathFromUri(song_.url()));
          if (!cover.data.empty()) {
            *embedded = cover.data;
            *extension = "jpg";
            return true;
          }
        }
        break;
      case AlbumCoverLoaderOptions::Type::Manual:
        if (dialog_result_.export_downloaded && !song_.art_manual().empty()) {
          const std::string cover_path = FileUtils::PathFromUri(song_.art_manual());
          if (FileUtils::Exists(cover_path)) {
            *path = cover_path;
            *extension = FileUtils::Extension(cover_path);
            if (extension->empty()) {
              *extension = "jpg";
            }
            return true;
          }
        }
        break;
      case AlbumCoverLoaderOptions::Type::Automatic:
        if (dialog_result_.export_downloaded && !song_.art_automatic().empty()) {
          const std::string cover_path = FileUtils::PathFromUri(song_.art_automatic());
          if (FileUtils::Exists(cover_path)) {
            *path = cover_path;
            *extension = FileUtils::Extension(cover_path);
            if (extension->empty()) {
              *extension = "jpg";
            }
            return true;
          }
        }
        break;
    }
  }
  return false;
}

bool CoverExportRunnable::Run() {
  if (song_.art_unset() || (!song_.art_embedded() && song_.art_automatic().empty() && song_.art_manual().empty())) {
    return false;
  }
  if (dialog_result_.RequiresCoverProcessing()) {
    return ProcessAndExportCover();
  }
  return ExportCover();
}

bool CoverExportRunnable::ExportCover() {
  std::string source;
  std::string extension;
  std::vector<unsigned char> embedded;
  if (!LoadSource(&source, &extension, &embedded)) {
    return false;
  }
  const std::string dest = DestinationPath(song_, dialog_result_, extension);
  if (dialog_result_.overwrite == AlbumCoverExport::OverwriteMode::None && FileUtils::Exists(dest)) {
    return false;
  }
  if (FileUtils::Exists(dest) && dialog_result_.overwrite != AlbumCoverExport::OverwriteMode::None && !FileUtils::Remove(dest)) {
    return false;
  }
  if (!embedded.empty()) {
    return FileUtils::WriteFile(dest, std::string(embedded.begin(), embedded.end()));
  }
  return FileUtils::CopyFile(source, dest);
}

bool CoverExportRunnable::ProcessAndExportCover() {
  std::string source;
  std::string extension;
  std::vector<unsigned char> embedded;
  if (!LoadSource(&source, &extension, &embedded)) {
    return false;
  }

  GdkPixbuf *pixbuf = nullptr;
  GError *error = nullptr;
  if (!embedded.empty()) {
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    gdk_pixbuf_loader_write(loader, embedded.data(), embedded.size(), &error);
    gdk_pixbuf_loader_close(loader, error ? nullptr : &error);
    if (!error) {
      pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
      if (pixbuf) {
        g_object_ref(pixbuf);
      }
    }
    g_object_unref(loader);
  } else {
    pixbuf = gdk_pixbuf_new_from_file(source.c_str(), &error);
  }
  if (error) {
    g_error_free(error);
    error = nullptr;
  }
  if (!pixbuf) {
    return false;
  }

  if (dialog_result_.IsSizeForced()) {
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, dialog_result_.width, dialog_result_.height, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    pixbuf = scaled;
  }

  const std::string dest = DestinationPath(song_, dialog_result_, extension);
  if (dialog_result_.overwrite == AlbumCoverExport::OverwriteMode::None && FileUtils::Exists(dest)) {
    g_object_unref(pixbuf);
    return false;
  }
  if (dialog_result_.overwrite == AlbumCoverExport::OverwriteMode::Smaller && FileUtils::Exists(dest)) {
    int existing_width = 0;
    int existing_height = 0;
    gdk_pixbuf_get_file_info(dest.c_str(), &existing_width, &existing_height);
    if (existing_width >= gdk_pixbuf_get_width(pixbuf) || existing_height >= gdk_pixbuf_get_height(pixbuf)) {
      g_object_unref(pixbuf);
      return false;
    }
  }
  if (FileUtils::Exists(dest) && dialog_result_.overwrite != AlbumCoverExport::OverwriteMode::None && !FileUtils::Remove(dest)) {
    g_object_unref(pixbuf);
    return false;
  }

  const bool saved = gdk_pixbuf_save(pixbuf, dest.c_str(), extension == "png" ? "png" : "jpeg", &error, nullptr);
  g_object_unref(pixbuf);
  if (error) {
    g_error_free(error);
  }
  return saved;
}
