#include "engine/ebur128analysis.h"

#include "config.h"
#include "utilities/audioanalysis.h"
#include "utilities/fileutils.h"

#ifdef HAVE_EBUR128
#include <ebur128.h>
#endif

std::optional<EBUR128Measures> EBUR128Analysis::Compute(const Song &song) {
  const std::string url = song.url();
  if (url.empty()) {
    return std::nullopt;
  }
#ifdef HAVE_EBUR128
  const std::vector<int16_t> pcm = AudioAnalysis::DecodePcm(url, 44100 * 120);
  if (pcm.size() < 1024) {
    return std::nullopt;
  }
  ebur128_state *state = ebur128_init(1, 44100, EBUR128_MODE_I | EBUR128_MODE_LRA);
  if (!state) {
    return std::nullopt;
  }
  ebur128_add_frames_short(state, pcm.data(), pcm.size());
  EBUR128Measures measures;
  double loudness = 0;
  if (ebur128_loudness_global(state, &loudness) == EBUR128_SUCCESS) {
    measures.loudness_lufs = loudness;
  }
  double range = 0;
  if (ebur128_loudness_range(state, &range) == EBUR128_SUCCESS) {
    measures.range_lu = range;
  }
  ebur128_destroy(&state);
  if (!measures.loudness_lufs) {
    return std::nullopt;
  }
  return measures;
#else
  (void)song;
  return std::nullopt;
#endif
}
