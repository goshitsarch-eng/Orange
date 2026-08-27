#ifndef STRAWBERRY_ERRORDIALOG_H
#define STRAWBERRY_ERRORDIALOG_H

#include <adwaita.h>
#include <gtk/gtk.h>

#include <string>
#include <vector>

class ErrorDialog {
 public:
  static void Show(GtkWindow *parent, const std::string &message);
};

// Qt ErrorDialog: accumulate messages and keep the dialog until the user closes it.
class QueuedErrorDialog {
 public:
  explicit QueuedErrorDialog(GtkWindow *parent);
  ~QueuedErrorDialog();

  QueuedErrorDialog(const QueuedErrorDialog &) = delete;
  QueuedErrorDialog &operator=(const QueuedErrorDialog &) = delete;

  void ShowMessage(const std::string &message);
  void Present();
  bool visible() const { return visible_; }
  bool active() const { return active_; }
  const std::vector<std::string> &messages() const { return messages_; }

 private:
  void EnsureDialog();
  void RefreshBody();
  void OnClosed();

  GtkWindow *parent_ = nullptr;
  AdwAlertDialog *dialog_ = nullptr;
  std::vector<std::string> messages_;
  bool visible_ = false;
  bool active_ = false;
};

#endif
