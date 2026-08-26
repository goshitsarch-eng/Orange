#ifndef STRAWBERRY_SCOPEDGOBJECT_H
#define STRAWBERRY_SCOPEDGOBJECT_H

#include <glib-object.h>

template <typename T>
class ScopedGObject {
 public:
  explicit ScopedGObject(T *object = nullptr) : object_(object) {}
  ~ScopedGObject() {
    if (object_) {
      g_object_unref(object_);
    }
  }
  T *get() const { return object_; }

 private:
  T *object_ = nullptr;
};

#endif
