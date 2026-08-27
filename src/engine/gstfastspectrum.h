#ifndef STRAWBERRY_GSTFASTSPECTRUM_H
#define STRAWBERRY_GSTFASTSPECTRUM_H

#include "config.h"

#include <cstdint>
#include <functional>
#include <vector>

class GstFastSpectrum {
 public:
  using OutputCallback = std::function<void(const double *magnitudes, int size)>;

  GstFastSpectrum();
  ~GstFastSpectrum();

  void set_bands(unsigned bands);
  void set_interval_ns(uint64_t interval);
  void set_output_callback(OutputCallback callback);
  void Process(const float *samples, unsigned count);
  const std::vector<double> &magnitudes() const { return magnitudes_; }

 private:
  unsigned bands_ = 64;
  uint64_t interval_ns_ = 100000000;
  OutputCallback callback_;
  std::vector<double> magnitudes_;
#ifdef HAVE_GSTFASTSPECTRUM
  void *plan_ = nullptr;
  double *fft_in_ = nullptr;
  void *fft_out_ = nullptr;
#endif
};

void gst_strawberry_fastspectrum_register();

#endif
