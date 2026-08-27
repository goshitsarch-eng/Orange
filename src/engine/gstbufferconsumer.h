#ifndef STRAWBERRY_GSTBUFFERCONSUMER_H
#define STRAWBERRY_GSTBUFFERCONSUMER_H

#include <gst/gst.h>

#include <string>

class GstBufferConsumer {
 public:
  virtual ~GstBufferConsumer() = default;
  virtual void ConsumeBuffer(GstBuffer *buffer, int pipeline_id, const std::string &format) = 0;
};

#endif
