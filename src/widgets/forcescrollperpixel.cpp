#include "widgets/forcescrollperpixel.h"

void ForceScrollPerPixel::Apply(GtkScrolledWindow *window) {
  if (!window) {
    return;
  }
  gtk_scrolled_window_set_kinetic_scrolling(window, TRUE);
  gtk_scrolled_window_set_overlay_scrolling(window, TRUE);
}
