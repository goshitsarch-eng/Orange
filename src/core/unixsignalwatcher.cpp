#include "core/unixsignalwatcher.h"

#include <glib-unix.h>

namespace {

struct SignalWatch {
  UnixSignalWatcher *self = nullptr;
  int signum = 0;
};

}  // namespace

UnixSignalWatcher::UnixSignalWatcher() = default;

UnixSignalWatcher::~UnixSignalWatcher() {
  for (const Entry &watch : watches_) {
    if (watch.id) {
      g_source_remove(watch.id);
    }
    delete static_cast<SignalWatch *>(watch.payload);
  }
}

void UnixSignalWatcher::Watch(int signum) {
  auto *payload = new SignalWatch{this, signum};
  Entry watch;
  watch.signum = signum;
  watch.payload = payload;
  watch.id = g_unix_signal_add(
      signum, +[](gpointer data) -> gboolean {
        auto *watch_data = static_cast<SignalWatch *>(data);
        if (watch_data && watch_data->self) {
          watch_data->self->Fired.Emit(watch_data->signum);
        }
        return G_SOURCE_CONTINUE;
      },
      payload);
  watches_.push_back(watch);
}
