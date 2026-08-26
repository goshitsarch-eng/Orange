#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class ClickableLabel {
 public:
  explicit ClickableLabel(const std::string &text = {});
  ~ClickableLabel();

  ClickableLabel(const ClickableLabel &) = delete;
  ClickableLabel &operator=(const ClickableLabel &) = delete;

  GtkWidget *widget() const { return button_; }

  void SetText(const std::string &text);
  std::string text() const;
  void SetClickedCallback(std::function<void()> callback) { clicked_cb_ = std::move(callback); }

 private:
  GtkWidget *button_ = nullptr;
  GtkWidget *label_ = nullptr;
  std::function<void()> clicked_cb_;
};

#endif  // CLICKABLELABEL_H
