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
    // Iterate a copy.
    // A slot is free to connect another slot, or to destroy whatever owns this signal, and either would
    // leave a loop over slots_ walking a vector that has been reallocated or freed.
    const std::vector<Slot> slots = slots_;
    for (const Slot &slot : slots) {
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
