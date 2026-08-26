#ifndef STRAWBERRY_DIALOGS_H
#define STRAWBERRY_DIALOGS_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class Application;
class Equalizer;

class Dialogs {
 public:
  static void AddStream(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback);
  static void CoverManager(GtkWindow *parent, Application *app);
  static void Equalizer(GtkWindow *parent, class Equalizer *equalizer);
  static void Transcode(GtkWindow *parent, Application *app);
  static void Organize(GtkWindow *parent, Application *app);
  static void TagFetcher(GtkWindow *parent, Application *app);
  static void EditTag(GtkWindow *parent, Application *app);
  static void Shortcuts(GtkWindow *parent);
  static void GrabShortcut(GtkWindow *parent, const std::function<void(const std::string &)> &callback);
  static void Login(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback);
  static void SmartPlaylistWizard(GtkWindow *parent, Application *app);
  static void GroupBy(GtkWindow *parent, const std::function<void(const std::string &)> &callback);
  static void Console(GtkWindow *parent);
  static void Error(GtkWindow *parent, const std::string &message);
};

#endif
