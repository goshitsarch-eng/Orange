#ifndef UIERROR_H
#define UIERROR_H

#include "core/signal.h"

#include <string>

namespace UiError {

// Shared bus for cover / edit-tag / tag-fetch failures that Qt shows via ShowErrorDialog.
inline Signal<std::string> &Bus() {
  static Signal<std::string> signal;
  return signal;
}

inline void Report(const std::string &message) {
  if (!message.empty()) {
    Bus().Emit(message);
  }
}

}  // namespace UiError

#endif  // UIERROR_H
