#ifndef STRAWBERRY_MOODBARITEMDELEGATE_H
#define STRAWBERRY_MOODBARITEMDELEGATE_H

#include "moodbar/moodbarrenderer.h"

#include <gtk/gtk.h>

class MoodbarItemDelegate {
 public:
  static void Paint(GtkSnapshot *snapshot, int width, int height, const std::vector<uint8_t> &mood);
};

#endif
