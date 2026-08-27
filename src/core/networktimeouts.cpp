#include "core/networktimeouts.h"

#include "core/network.h"

#include <glib.h>

#include <algorithm>

namespace {

struct TimeoutData {
  NetworkTimeouts *self = nullptr;
  int id = 0;
};

}  // namespace

void NetworkTimeouts::AddReply(int id) {
  if (id <= 0) {
    return;
  }
  Cancel(id);
  auto *data = new TimeoutData{this, id};
  const guint interval = static_cast<guint>(std::max(1, timeout_msec_));
  timers_[id] = g_timeout_add_full(
      G_PRIORITY_DEFAULT, interval,
      +[](gpointer user) -> gboolean {
        auto *data = static_cast<TimeoutData *>(user);
        NetworkTimeouts *self = data->self;
        const int reply_id = data->id;
        if (self) {
          self->timers_.erase(reply_id);
          if (self->abort_) {
            self->abort_(reply_id);
          }
        }
        return G_SOURCE_REMOVE;
      },
      data, +[](gpointer user) { delete static_cast<TimeoutData *>(user); });
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
  const std::map<int, unsigned> timers = timers_;
  timers_.clear();
  for (const auto &entry : timers) {
    if (entry.second) {
      g_source_remove(entry.second);
    }
  }
}

bool NetworkTimeouts::Contains(int id) const { return timers_.count(id) > 0; }

void NetworkTimeouts::Watch(NetworkAccessManager *network, int id) {
  if (!network || id <= 0) {
    return;
  }
  SetAbort([network](int reply_id) { network->Cancel(reply_id); });
  AddReply(id);
}
