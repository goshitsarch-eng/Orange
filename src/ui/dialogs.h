#ifndef STRAWBERRY_DIALOGS_H
#define STRAWBERRY_DIALOGS_H

#include "collection/collectiongrouping.h"
#include "core/song.h"
#include "dialogs/deletefilespolicy.h"
#include "smartplaylists/smartplaylist.h"

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class Application;
class Equalizer;

class Dialogs {
 public:
  static void AddStream(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback);
  static void CoverManager(GtkWindow *parent, Application *app);
  static void CoverFromUrl(GtkWindow *parent, Application *app);
  static void CoverSearch(GtkWindow *parent, Application *app);
  static void CoverExport(GtkWindow *parent, Application *app);
  static void Equalizer(GtkWindow *parent, Application *app);
  static void Transcode(GtkWindow *parent, Application *app);
  static void Organize(GtkWindow *parent, Application *app, const SongList &songs = {}, bool move = false);
  static void TagFetcher(GtkWindow *parent, Application *app);
  static void EditTag(GtkWindow *parent, Application *app, const SongList &songs = {});
  static void Shortcuts(GtkWindow *parent);
  static void GrabShortcut(GtkWindow *parent, const std::function<void(const std::string &)> &callback, const std::string &action = {});
  static void Login(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback);
  static void SmartPlaylistWizard(GtkWindow *parent, Application *app);
  static void SmartPlaylistWizard(GtkWindow *parent, Application *app, const std::string &name, const SmartPlaylistSearch &search);
  static void GroupBy(GtkWindow *parent, const CollectionGrouping::Grouping &current,
                      const std::function<void(const CollectionGrouping::Grouping &)> &callback);
  static void ManageSavedGroupings(GtkWindow *parent, const std::function<void(const CollectionGrouping::Grouping &)> &callback);
  static void PlaylistColumns(GtkWindow *parent, const std::function<void()> &callback);
  static void DeleteFiles(GtkWindow *parent, Application *app, const SongList &songs = {},
                          DeleteFilesPolicy::Source source = DeleteFilesPolicy::Source::Playlist);
  static void CopyToDevice(GtkWindow *parent, Application *app, const SongList &songs = {}, const std::string &playlist = {});
  static void CopyToDevice(GtkWindow *parent, Application *app, const std::vector<std::string> &filenames);
  static void SaveAllPlaylists(GtkWindow *parent, Application *app);
  static void Console(GtkWindow *parent, Application *app = nullptr);
  static void Error(GtkWindow *parent, const std::string &message);
};

#endif
