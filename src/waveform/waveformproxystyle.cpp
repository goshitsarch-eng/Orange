#include "waveform/waveformproxystyle.h"

void WaveformProxyStyle::Apply(GtkWidget *widget) {
  if (widget) {
    gtk_widget_add_css_class(widget, "waveform");
  }
}
