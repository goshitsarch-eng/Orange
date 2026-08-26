#ifndef STRAWBERRY_LAZY_H
#define STRAWBERRY_LAZY_H

#include <functional>
#include <memory>

template <typename T>
class Lazy {
 public:
  explicit Lazy(std::function<T *()> factory) : factory_(std::move(factory)) {}
  T *get() {
    if (!value_) {
      value_.reset(factory_ ? factory_() : new T());
    }
    return value_.get();
  }
  T *operator->() { return get(); }

 private:
  std::function<T *()> factory_;
  std::unique_ptr<T> value_;
};

#endif
