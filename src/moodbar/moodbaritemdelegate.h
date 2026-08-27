#ifndef STRAWBERRY_MOODBARITEMDELEGATE_H
#define STRAWBERRY_MOODBARITEMDELEGATE_H

#include "core/song.h"
#include "moodbar/moodbar.h"
#include "moodbar/moodbarcell.h"
#include "moodbar/moodbarrenderer.h"

#include <cairo.h>
#include <gtk/gtk.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class MoodbarItemDelegate {
 public:
  MoodbarItemDelegate();
  ~MoodbarItemDelegate();

  static void Paint(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood);

  const std::vector<uint8_t> *Peek(const std::string &url) const;
  const std::vector<uint8_t> &Ensure(const Song &song);
  void SetUpdatedCallback(const std::function<void()> &callback);
  void FinishGenerate(const std::string &key, std::vector<uint8_t> data);

 private:
  void StartGenerate(const Song &song);
  void MaybeStartNext();

  MoodbarLoader loader_;
  std::map<std::string, std::vector<uint8_t>> cache_;
  std::map<std::string, bool> loading_;
  std::vector<Song> pending_;
  int inflight_ = 0;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  std::function<void()> updated_;
};

#endif
