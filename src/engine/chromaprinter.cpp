#include "engine/chromaprinter.h"

#include "config.h"
#include "utilities/audioanalysis.h"
#include "utilities/fileutils.h"

#ifdef HAVE_CHROMAPRINT
#include <chromaprint.h>
#endif

Chromaprinter::Chromaprinter(const std::string &url_or_filename) : url_(url_or_filename) {
  if (url_.find("://") == std::string::npos) {
    url_ = FileUtils::UriFromPath(url_or_filename);
  }
}

std::string Chromaprinter::CreateFingerprint() { return CreateFingerprintInternal(true); }

std::string Chromaprinter::CreateFullFingerprint() { return CreateFingerprintInternal(false); }

std::string Chromaprinter::CreateFingerprintInternal(bool legacy) {
  last_error_.clear();
#ifdef HAVE_CHROMAPRINT
  const size_t max_samples = legacy ? static_cast<size_t>(11025 * 30) : static_cast<size_t>(44100 * 120);
  const std::vector<int16_t> pcm = AudioAnalysis::DecodePcm(url_, max_samples);
  if (pcm.empty()) {
    last_error_ = "Could not decode audio for fingerprint";
    return {};
  }
  ChromaprintContext *ctx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
  if (!ctx) {
    last_error_ = "Could not create Chromaprint context";
    return {};
  }
  if (!chromaprint_start(ctx, legacy ? 11025 : 44100, 1)) {
    last_error_ = "Chromaprint start failed";
    chromaprint_free(ctx);
    return {};
  }
  if (!chromaprint_feed(ctx, pcm.data(), static_cast<int>(pcm.size())) || !chromaprint_finish(ctx)) {
    last_error_ = "Chromaprint feed failed";
    chromaprint_free(ctx);
    return {};
  }
  char *encoded = nullptr;
  if (!chromaprint_get_fingerprint(ctx, &encoded)) {
    last_error_ = "Chromaprint encode failed";
    chromaprint_free(ctx);
    return {};
  }
  std::string fingerprint = encoded ? encoded : "";
  chromaprint_dealloc(encoded);
  chromaprint_free(ctx);
  return fingerprint;
#else
  (void)legacy;
  last_error_ = "Chromaprint is not available";
  return {};
#endif
}
