#include "utilities/threadutils.h"

#include <pthread.h>

std::string ThreadUtils::CurrentName() {
  char name[16] = {};
  pthread_getname_np(pthread_self(), name, sizeof(name));
  return name;
}

void ThreadUtils::SetName(const std::string &name) { pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()); }
