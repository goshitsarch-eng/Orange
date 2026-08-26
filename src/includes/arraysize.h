#ifndef STRAWBERRY_ARRAYSIZE_H
#define STRAWBERRY_ARRAYSIZE_H

#include <cstddef>

template <typename T, size_t N>
constexpr size_t arraysize(T (&)[N]) {
  return N;
}

#endif
