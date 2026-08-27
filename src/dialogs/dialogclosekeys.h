#ifndef STRAWBERRY_DIALOGCLOSEKEYS_H
#define STRAWBERRY_DIALOGCLOSEKEYS_H

#include <adwaita.h>

namespace DialogCloseKeys {

// Qt QKeySequence::Close is Ctrl+W (Equalizer, AlbumCoverManager).
constexpr unsigned kW = 'w';
constexpr unsigned kWUpper = 'W';
constexpr unsigned kControlMask = 1u << 2;

inline bool IsClose(unsigned keyval, unsigned state) {
  return (keyval == kW || keyval == kWUpper) && (state & kControlMask) != 0;
}

inline void Attach(AdwDialog *dialog) {
  if (!dialog) {
    return;
  }
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(GTK_WIDGET(dialog), keys);
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     if (!IsClose(keyval, static_cast<unsigned>(state))) {
                       return FALSE;
                     }
                     adw_dialog_close(ADW_DIALOG(data));
                     return TRUE;
                   })),
                   dialog);
}

}  // namespace DialogCloseKeys

#endif
