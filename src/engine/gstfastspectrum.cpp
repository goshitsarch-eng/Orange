#include "engine/gstfastspectrum.h"

#include <algorithm>
#include <cmath>

#ifdef HAVE_GSTFASTSPECTRUM
#include <fftw3.h>
#endif

GstFastSpectrum::GstFastSpectrum() { set_bands(64); }

GstFastSpectrum::~GstFastSpectrum() {
#ifdef HAVE_GSTFASTSPECTRUM
  if (plan_) {
    fftw_destroy_plan(static_cast<fftw_plan>(plan_));
  }
  fftw_free(fft_in_);
  fftw_free(fft_out_);
#endif
}

void GstFastSpectrum::set_bands(unsigned bands) {
  bands_ = bands > 0 ? bands : 64;
  magnitudes_.assign(bands_, 0.0);
#ifdef HAVE_GSTFASTSPECTRUM
  if (plan_) {
    fftw_destroy_plan(static_cast<fftw_plan>(plan_));
    plan_ = nullptr;
  }
  fftw_free(fft_in_);
  fftw_free(fft_out_);
  fft_in_ = static_cast<double *>(fftw_malloc(sizeof(double) * bands_ * 2));
  fft_out_ = fftw_malloc(sizeof(fftw_complex) * (bands_ + 1));
  if (fft_in_ && fft_out_) {
    plan_ = fftw_plan_dft_r2c_1d(static_cast<int>(bands_ * 2), fft_in_, static_cast<fftw_complex *>(fft_out_), FFTW_ESTIMATE);
  }
#endif
}

void GstFastSpectrum::set_interval_ns(uint64_t interval) { interval_ns_ = interval; }

void GstFastSpectrum::set_output_callback(OutputCallback callback) { callback_ = std::move(callback); }

void GstFastSpectrum::Process(const float *samples, unsigned count) {
  if (!samples || count == 0) {
    return;
  }
#ifdef HAVE_GSTFASTSPECTRUM
  if (fft_in_ && plan_) {
    const unsigned n = std::min(count, bands_ * 2);
    for (unsigned i = 0; i < bands_ * 2; ++i) {
      fft_in_[i] = i < n ? samples[i] : 0.0;
    }
    fftw_execute(static_cast<fftw_plan>(plan_));
    auto *out = static_cast<fftw_complex *>(fft_out_);
    for (unsigned i = 0; i < bands_; ++i) {
      magnitudes_[i] = std::hypot(out[i][0], out[i][1]);
    }
  } else
#endif
  {
    for (unsigned i = 0; i < bands_; ++i) {
      const unsigned index = (i * count) / bands_;
      magnitudes_[i] = std::fabs(samples[index]);
    }
  }
  if (callback_) {
    callback_(magnitudes_.data(), static_cast<int>(magnitudes_.size()));
  }
}

void gst_strawberry_fastspectrum_register() {}
