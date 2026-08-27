#ifndef STRAWBERRY_SIGNALCHECKER_H
#define STRAWBERRY_SIGNALCHECKER_H

#include <glib-object.h>

inline gulong CheckedConnect(gpointer instance, const char *signal, GCallback callback, gpointer data) {
  return g_signal_connect(instance, signal, callback, data);
}

#endif
