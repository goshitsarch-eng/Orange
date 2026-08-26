#include "moodbar/moodbarproxystyle.h"

void MoodbarProxyStyle::Apply(GtkWidget *widget) {
  if (widget) {
    gtk_widget_add_css_class(widget, "moodbar");
  }
}
