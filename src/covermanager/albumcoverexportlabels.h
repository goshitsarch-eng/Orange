#ifndef STRAWBERRY_ALBUMCOVEREXPORTLABELS_H
#define STRAWBERRY_ALBUMCOVEREXPORTLABELS_H

#include "core/settings.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverloaderoptions.h"

#include <string>

namespace AlbumCoverExportLabels {

constexpr char kSettingsGroup[] = "AlbumCoverExport";
constexpr char kFileName[] = "fileName";
constexpr char kOverwrite[] = "overwrite";
constexpr char kForceSize[] = "forceSize";
constexpr char kWidth[] = "width";
constexpr char kHeight[] = "height";
constexpr char kExportDownloaded[] = "export_downloaded";
constexpr char kExportEmbedded[] = "export_embedded";

inline const char *Title() { return "Export covers"; }
inline const char *FilenamePrompt() { return "Enter a filename for exported covers (no extension):"; }
inline const char *ExportDownloaded() { return "Export downloaded covers"; }
inline const char *ExportEmbedded() { return "Export embedded covers"; }
inline const char *ExistingCovers() { return "Existing covers"; }
inline const char *DoNotOverwrite() { return "Do not overwrite"; }
inline const char *OverwriteAll() { return "Overwrite all"; }
inline const char *OverwriteSmaller() { return "Overwrite smaller ones only"; }
inline const char *ScaleSize() { return "Scale size"; }
inline const char *Size() { return "Size:"; }
inline const char *Pixel() { return "Pixel"; }
inline const char *Output() { return "Output"; }
inline const char *Export() { return "Export"; }
inline const char *DefaultFilename() { return "cover"; }

inline const char *OverwriteLabel(AlbumCoverExport::OverwriteMode mode) {
  switch (mode) {
    case AlbumCoverExport::OverwriteMode::All:
      return OverwriteAll();
    case AlbumCoverExport::OverwriteMode::Smaller:
      return OverwriteSmaller();
    case AlbumCoverExport::OverwriteMode::None:
    default:
      return DoNotOverwrite();
  }
}

inline AlbumCoverExport::OverwriteMode OverwriteFromInt(int value) {
  if (value == static_cast<int>(AlbumCoverExport::OverwriteMode::All)) {
    return AlbumCoverExport::OverwriteMode::All;
  }
  if (value == static_cast<int>(AlbumCoverExport::OverwriteMode::Smaller)) {
    return AlbumCoverExport::OverwriteMode::Smaller;
  }
  return AlbumCoverExport::OverwriteMode::None;
}

inline int OverwriteRadioIndex(AlbumCoverExport::OverwriteMode mode) { return static_cast<int>(mode); }

inline AlbumCoverExport::OverwriteMode OverwriteFromRadio(int index) { return OverwriteFromInt(index); }

inline bool ForceSizeEnabled(bool forcesize) { return forcesize; }

inline AlbumCoverExport::DialogResult Defaults() {
  AlbumCoverExport::DialogResult result;
  result.filename = DefaultFilename();
  result.export_downloaded = true;
  result.export_embedded = false;
  result.overwrite = AlbumCoverExport::OverwriteMode::None;
  return result;
}

inline AlbumCoverLoaderOptions::Types TypesFor(const AlbumCoverExport::DialogResult &result) {
  AlbumCoverLoaderOptions::Types types;
  if (result.export_embedded) {
    types.push_back(AlbumCoverLoaderOptions::Type::Embedded);
  }
  if (result.export_downloaded) {
    types.push_back(AlbumCoverLoaderOptions::Type::Automatic);
    types.push_back(AlbumCoverLoaderOptions::Type::Manual);
  }
  return types;
}

inline AlbumCoverExport::DialogResult FromSettings(Settings *settings) {
  AlbumCoverExport::DialogResult result = Defaults();
  if (!settings) {
    return result;
  }
  settings->BeginGroup(kSettingsGroup);
  result.filename = settings->Value(kFileName, DefaultFilename());
  if (result.filename.empty()) {
    result.filename = DefaultFilename();
  }
  result.overwrite = OverwriteFromInt(settings->IntValue(kOverwrite, static_cast<int>(AlbumCoverExport::OverwriteMode::None)));
  result.forcesize = settings->BoolValue(kForceSize, false);
  result.width = settings->IntValue(kWidth, 0);
  result.height = settings->IntValue(kHeight, 0);
  result.export_downloaded = settings->BoolValue(kExportDownloaded, true);
  result.export_embedded = settings->BoolValue(kExportEmbedded, false);
  return result;
}

inline void ApplyToSettings(Settings *settings, const AlbumCoverExport::DialogResult &result) {
  if (!settings) {
    return;
  }
  settings->BeginGroup(kSettingsGroup);
  settings->SetValue(kFileName, result.filename.empty() ? DefaultFilename() : result.filename);
  settings->SetIntValue(kOverwrite, static_cast<int>(result.overwrite));
  settings->SetBoolValue(kForceSize, result.forcesize);
  settings->SetIntValue(kWidth, result.width);
  settings->SetIntValue(kHeight, result.height);
  settings->SetBoolValue(kExportDownloaded, result.export_downloaded);
  settings->SetBoolValue(kExportEmbedded, result.export_embedded);
  settings->Sync();
}

}  // namespace AlbumCoverExportLabels

#endif
