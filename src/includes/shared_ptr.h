#ifndef STRAWBERRY_SHARED_PTR_H
#define STRAWBERRY_SHARED_PTR_H

#include <memory>

template <typename T>
using SharedPtr = std::shared_ptr<T>;

#endif
