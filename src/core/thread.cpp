#include "core/thread.h"

Thread::Thread(std::function<void()> work) : thread_(std::move(work)) {}

Thread::~Thread() { Join(); }

void Thread::Join() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

bool Thread::joinable() const { return thread_.joinable(); }
