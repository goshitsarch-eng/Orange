#ifndef STRAWBERRY_TAGREADERCLIENTPUMP_H
#define STRAWBERRY_TAGREADERCLIENTPUMP_H

namespace TagReaderClientPump {

inline bool ShouldArm(bool have_requests, bool already_armed) { return have_requests && !already_armed; }

inline bool ShouldContinue(bool have_requests) { return have_requests; }

}  // namespace TagReaderClientPump

#endif  // STRAWBERRY_TAGREADERCLIENTPUMP_H
