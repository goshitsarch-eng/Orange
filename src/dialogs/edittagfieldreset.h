#ifndef DIALOGS_EDITTAGFIELDRESET_H_
#define DIALOGS_EDITTAGFIELDRESET_H_

#include <string>

namespace EditTagFieldReset {

inline bool ShouldShowReset(const std::string &current, const std::string &initial) { return current != initial; }

inline bool ShouldShowReset(double current, double initial) { return current != initial; }

inline std::string ResetValue(const std::string &initial) { return initial; }

inline const char *ResetTooltip() { return "Reset this field"; }

}  // namespace EditTagFieldReset

#endif  // DIALOGS_EDITTAGFIELDRESET_H_
