#ifndef STRAWBERRY_SCOPED_PTR_H
#define STRAWBERRY_SCOPED_PTR_H

#include <memory>

template <typename T>
using ScopedPtr = std::unique_ptr<T>;

#endif
