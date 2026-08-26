#ifndef STRAWBERRY_UNIXSIGNALWATCHER_H
#define STRAWBERRY_UNIXSIGNALWATCHER_H

#include "core/signal.h"

#include <glib.h>

#include <vector>

class UnixSignalWatcher {
 public:
  UnixSignalWatcher();
  ~UnixSignalWatcher();

  void Watch(int signum);
  Signal<int> Fired;

 private:
  struct Entry {
    int signum = 0;
    guint id = 0;
    void *payload = nullptr;
  };
  std::vector<Entry> watches_;
};

#endif
