#include "core/networktimeouts.h"

#include <glib.h>

void NetworkTimeouts::AddReply(int id) {
  Cancel(id);
  timers_[id] = g_timeout_add(timeout_msec_, [](gpointer) -> gboolean { return G_SOURCE_REMOVE; }, nullptr);
}

void NetworkTimeouts::Cancel(int id) {
  auto it = timers_.find(id);
  if (it == timers_.end()) {
    return;
  }
  if (it->second) {
    g_source_remove(it->second);
  }
  timers_.erase(it);
}

void NetworkTimeouts::CancelAll() {
  for (auto &entry : timers_) {
    if (entry.second) {
      g_source_remove(entry.second);
    }
  }
  timers_.clear();
}

bool NetworkTimeouts::Contains(int id) const { return timers_.count(id) > 0; }
