#ifndef SEARCHFIELD_H
#define SEARCHFIELD_H

#include <functional>
#include <string>

#include <gtk/gtk.h>

class SearchField {
 public:
  explicit SearchField();
  ~SearchField();

  SearchField(const SearchField &) = delete;
  SearchField &operator=(const SearchField &) = delete;

  GtkWidget *widget() const { return entry_; }

  std::string text() const;
  void SetText(const std::string &text);
  void Clear();

  void SetChangedCallback(std::function<void(const std::string &)> callback) { changed_cb_ = std::move(callback); }
  void SetActivatedCallback(std::function<void(const std::string &)> callback) { activated_cb_ = std::move(callback); }

 private:
  GtkWidget *entry_ = nullptr;
  std::function<void(const std::string &)> changed_cb_;
  std::function<void(const std::string &)> activated_cb_;
};

#endif  // SEARCHFIELD_H
