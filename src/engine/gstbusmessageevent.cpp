#include "engine/gstbusmessageevent.h"

GstBusMessageEvent::GstBusMessageEvent(GstMessage *message, uint64_t generation) : generation_(generation) {
  if (message) {
    message_ = gst_message_ref(message);
  }
}

GstBusMessageEvent::~GstBusMessageEvent() {
  if (message_) {
    gst_message_unref(message_);
  }
}
