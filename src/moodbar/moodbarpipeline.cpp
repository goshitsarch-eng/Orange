#include "moodbar/moodbarpipeline.h"

#include "moodbar/moodbarbuilder.h"
#include "utilities/audioanalysis.h"

std::vector<uint8_t> MoodbarPipeline::Run(const std::string &url) {
  const std::vector<int16_t> pcm = AudioAnalysis::DecodePcm(url);
  if (pcm.empty()) {
    return {};
  }
  return MoodbarBuilder::FromPcm(pcm.data(), pcm.size());
}
