#ifndef STRAWBERRY_ALBUMCOVERCHOICECONTROLLER_H
#define STRAWBERRY_ALBUMCOVERCHOICECONTROLLER_H

#include "core/song.h"
#include "covermanager/coversearchstatistics.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class Application;

class AlbumCoverChoiceController {
 public:
  explicit AlbumCoverChoiceController(Application *app);

  void FetchCover(Song *song, GtkWidget *image = nullptr, GtkWidget *status = nullptr, std::function<void(bool)> done = {});
  void SearchForCover(GtkWindow *parent);
  void UnsetCover(Song *song, GtkWidget *image = nullptr);
  void ClearCover(Song *song, GtkWidget *image = nullptr);
  void DeleteCover(Song *song, GtkWidget *image = nullptr);
  void LoadCoverFromURL(GtkWindow *parent, Song *song, GtkWidget *image = nullptr);
  void LoadCoverFromFile(GtkWindow *parent, Song *song, GtkWidget *image = nullptr);
  void SaveCoverToFile(GtkWindow *parent, const Song &song);
  void ShowCover(GtkWindow *parent, const Song &song);
  void SearchCoverAutomatically(Song *song, GtkWidget *image = nullptr);
  void ShowStatistics(GtkWindow *parent);
  void AttachMenu(GtkWidget *widget, GtkWindow *parent, const std::function<Song()> &song_for_menu);

  const CoverSearchStatistics &statistics() const { return statistics_; }

  static bool IsKnownImageExtension(const std::string &extension);
  static std::vector<std::string> ImageExtensions();
  static bool SaveCover(Application *app, Song *song, const std::string &image);

 private:
  void ApplyImage(Song *song, GtkWidget *image, const std::string &data);

  Application *app_ = nullptr;
  CoverSearchStatistics statistics_;
};

#endif
