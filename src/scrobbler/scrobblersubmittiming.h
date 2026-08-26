#ifndef STRAWBERRY_SCROBBLERSUBMITTIMING_H
#define STRAWBERRY_SCROBBLERSUBMITTIMING_H

#include <algorithm>

namespace ScrobblerSubmitTiming {

inline int DelaySeconds(int configured, bool had_error) {
  if (configured <= 0) {
    return 0;
  }
  return std::max(configured, had_error ? 30 : 5);
}

}  // namespace ScrobblerSubmitTiming

#endif
