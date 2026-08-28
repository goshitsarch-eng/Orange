#ifndef STRAWBERRY_WINBLURBEHIND_H
#define STRAWBERRY_WINBLURBEHIND_H

namespace WinBlurBehind {

// Qt OSDPretty::Reposition applies DWM blur-behind on the rounded mask.
inline bool ShouldApply(bool is_windows, bool has_region) { return is_windows && has_region; }

}  // namespace WinBlurBehind

#endif
