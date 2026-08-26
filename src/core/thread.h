#ifndef STRAWBERRY_THREAD_H
#define STRAWBERRY_THREAD_H

#include <functional>
#include <thread>

class Thread {
 public:
  explicit Thread(std::function<void()> work);
  ~Thread();
  void Join();
  bool joinable() const;

 private:
  std::thread thread_;
};

#endif
