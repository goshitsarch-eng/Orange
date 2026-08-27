#ifndef STRAWBERRY_FORCESCROLLPERPIXEL_H
#define STRAWBERRY_FORCESCROLLPERPIXEL_H

#include <gtk/gtk.h>

class ForceScrollPerPixel {
 public:
  static void Apply(GtkScrolledWindow *window);
};

#endif
