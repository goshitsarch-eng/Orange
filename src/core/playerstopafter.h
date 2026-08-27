#ifndef STRAWBERRY_PLAYERSTOPAFTER_H
#define STRAWBERRY_PLAYERSTOPAFTER_H

namespace PlayerStopAfter {

inline bool ShouldPrepareResume(bool stop_after) { return stop_after; }

inline int ResumeRow(int next_row) { return next_row; }

}  // namespace PlayerStopAfter

#endif  // STRAWBERRY_PLAYERSTOPAFTER_H
