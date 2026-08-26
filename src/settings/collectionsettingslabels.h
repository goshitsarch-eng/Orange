#ifndef STRAWBERRY_COLLECTIONSETTINGSLABELS_H
#define STRAWBERRY_COLLECTIONSETTINGSLABELS_H

namespace CollectionSettingsLabels {

inline const char *Intro() { return "These folders will be scanned for music to make up your collection"; }
inline const char *AutomaticUpdating() { return "Automatic updating"; }
inline const char *StartupScan() { return "Update the collection when Strawberry starts"; }
inline const char *Monitor() { return "Monitor the collection for changes"; }
inline const char *SongTracking() { return "Song fingerprinting and tracking"; }
inline const char *MarkUnavailable() { return "Mark disappeared songs unavailable"; }
inline const char *EbuAnalysis() { return "Perform song EBU R 128 analysis (required for EBU R 128 loudness normalization)"; }
inline const char *ExpireUnavailable() { return "Expire unavailable songs after"; }
inline const char *Days() { return "days"; }
inline const char *CoverPatterns() { return "Preferred album art filenames (comma separated)"; }
inline const char *DisplayOptions() { return "Display options"; }
inline const char *AutoOpen() { return "Automatically open single categories in the collection tree"; }
inline const char *ShowDividers() { return "Show dividers"; }
inline const char *PrettyCovers() { return "Show album cover art in collection"; }
inline const char *VariousArtists() { return "Use various artists for compilation albums"; }
inline const char *SkipArtistArticles() {
  return "Skip leading articles (\"the\", \"a\", \"an\") when sorting artists, composers and performers";
}
inline const char *SkipAlbumArticles() { return "Skip leading articles (\"the\", \"a\", \"an\") when sorting albums"; }
inline const char *UseSortTags() { return "Use sort tags for sorting when available"; }
inline const char *CacheGroup() { return "Album cover pixmap cache"; }
inline const char *PlaycountsGroup() { return "Song playcounts and ratings"; }
inline const char *AddFolder() { return "Add new folder..."; }
inline const char *RemoveFolder() { return "Remove folder"; }
inline const char *CoverPatternsHint() {
  return "When looking for album art Strawberry will first look for picture files that contain one of these words.\n"
         "If there are no matches then it will use the largest image in the directory.";
}
inline const char *CacheSize() { return "Size"; }
inline const char *EnableDiskCache() { return "Enable Disk Cache"; }
inline const char *DiskCacheSize() { return "Disk Cache Size"; }
inline const char *DeleteFiles() { return "Enable delete files in the right click context menu"; }

}  // namespace CollectionSettingsLabels

#endif
