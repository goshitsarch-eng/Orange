#ifndef STRAWBERRY_ALBUMCOVERCHOICECONTROLLER_H
#define STRAWBERRY_ALBUMCOVERCHOICECONTROLLER_H

#include "core/signal.h"
#include "core/song.h"
#include "covermanager/coverchoicemenu.h"
#include "covermanager/coveroptions.h"
#include "covermanager/coversearchstatistics.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class Application;

class AlbumCoverChoiceController {
 public:
  explicit AlbumCoverChoiceController(Application *app);

  // The fetch is asynchronous, so the song is taken by value and the result is handed back through the
  // callback. Holding a caller's Song* across the request meant the reply wrote into whatever had since
  // taken that address.
  using CoverFetchedCallback = std::function<void(bool ok, const Song &song)>;
  void FetchCover(const Song &song, GtkWidget *image = nullptr, GtkWidget *status = nullptr, CoverFetchedCallback done = {});
  void SearchForCover(GtkWindow *parent);
  void SearchForCover(GtkWindow *parent, const Song &song, std::function<void(bool)> done = {});
  void UnsetCover(Song *song, GtkWidget *image = nullptr);
  void ClearCover(Song *song, GtkWidget *image = nullptr);
  void DeleteCover(Song *song, GtkWidget *image = nullptr);
  void LoadCoverFromURL(GtkWindow *parent, Song *song, GtkWidget *image = nullptr);
  void LoadCoverFromFile(GtkWindow *parent, Song *song, GtkWidget *image = nullptr);
  void SaveCoverToFile(GtkWindow *parent, const Song &song);
  void ShowCover(GtkWindow *parent, const Song &song);
  void SearchCoverAutomatically(Song *song, GtkWidget *image = nullptr);
  void ShowStatistics(GtkWindow *parent);
  void AttachMenu(GtkWidget *widget, GtkWindow *parent, const std::function<Song()> &song_for_menu,
                  const std::function<bool()> &has_cover = {});
  void PopupAttachedMenu(GtkWidget *widget, GtkWindow *parent);
  gboolean OnAttachedKey(GtkWidget *widget, guint keyval, GdkModifierType state);
  void SetSearchAutoChangedCallback(std::function<void(bool)> callback) { search_auto_changed_ = std::move(callback); }
  void Perform(CoverChoiceMenu::Action action, GtkWindow *parent, Song *song, GtkWidget *image = nullptr);

  const CoverSearchStatistics &statistics() const { return statistics_; }
  void ResetStatistics() { statistics_ = {}; }

  void ReloadSettings();
  void set_save_embedded_cover_override(bool value) { save_embedded_cover_override_ = value; }
  CoverOptions::CoverType get_collection_save_album_cover_type() const { return cover_options_.cover_type; }
  CoverOptions::CoverType get_save_album_cover_type() const {
    return save_embedded_cover_override_ ? CoverOptions::CoverType::Embedded : cover_options_.cover_type;
  }
  CoverOptions EffectiveOptions() const {
    CoverOptions options = cover_options_;
    if (save_embedded_cover_override_) {
      options.cover_type = CoverOptions::CoverType::Embedded;
    }
    return options;
  }

  static bool IsKnownImageExtension(const std::string &extension);
  static std::vector<std::string> ImageExtensions();
  static bool SaveCover(Application *app, Song *song, const std::string &image);
  bool SaveCover(Song *song, const std::string &image);

  Signal<std::string> Error;

 private:
  void ApplyImage(Song *song, GtkWidget *image, const std::string &data);

  Application *app_ = nullptr;
  std::function<void(bool)> search_auto_changed_;
  CoverSearchStatistics statistics_;
  CoverOptions cover_options_;
  bool save_embedded_cover_override_ = false;
};

#endif
