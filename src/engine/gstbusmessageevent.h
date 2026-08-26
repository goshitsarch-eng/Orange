#ifndef STRAWBERRY_GSTBUSMESSAGEEVENT_H
#define STRAWBERRY_GSTBUSMESSAGEEVENT_H

#include <gst/gst.h>

#include <cstdint>

class GstBusMessageEvent {
 public:
  GstBusMessageEvent(GstMessage *message, uint64_t generation);
  ~GstBusMessageEvent();

  GstMessage *message() const { return message_; }
  uint64_t generation() const { return generation_; }

 private:
  GstMessage *message_ = nullptr;
  uint64_t generation_ = 0;
};

#endif
