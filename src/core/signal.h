#ifndef STRAWBERRY_SIGNAL_H
#define STRAWBERRY_SIGNAL_H

#include <functional>
#include <utility>
#include <vector>

template <typename... Args>
class Signal {
 public:
  using Slot = std::function<void(Args...)>;

  void Connect(Slot slot) { slots_.push_back(std::move(slot)); }

  void Emit(Args... args) const {
    for (const Slot &slot : slots_) {
      slot(args...);
    }
  }

  void operator()(Args... args) const { Emit(args...); }

  void Clear() { slots_.clear(); }

  bool empty() const { return slots_.empty(); }

 private:
  std::vector<Slot> slots_;
};

#endif
