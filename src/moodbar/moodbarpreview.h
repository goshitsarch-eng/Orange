#ifndef STRAWBERRY_MOODBARPREVIEW_H
#define STRAWBERRY_MOODBARPREVIEW_H

#include "constants/moodbarsettings.h"
#include "moodbar/moodbarstyle.h"

#include <gio/gio.h>

#include <cstdint>
#include <string>
#include <vector>

namespace MoodbarPreview {

// Qt MoodbarSettingsPage::InitMoodbarPreviews
inline constexpr int kWidth = 150;
inline constexpr int kHeight = 18;
inline const char *SampleResource() { return "/org/strawberrymusicplayer/Strawberry/mood/sample.mood"; }

inline std::vector<uint8_t> LoadSample() {
  GBytes *bytes = g_resources_lookup_data(SampleResource(), G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
  if (!bytes) {
    return {};
  }
  gsize size = 0;
  const guint8 *data = static_cast<const guint8 *>(g_bytes_get_data(bytes, &size));
  std::vector<uint8_t> out(data && size > 0 ? data : nullptr, data && size > 0 ? data + size : nullptr);
  g_bytes_unref(bytes);
  return out;
}

inline std::vector<uint8_t> Stripe(const std::vector<uint8_t> &mood, MoodbarSettings::Style style, int width) {
  if (width <= 0) {
    return {};
  }
  const std::vector<uint8_t> styled = MoodbarStyle::Apply(mood, style);
  const std::vector<uint8_t> &bytes = styled.empty() ? mood : styled;
  const size_t samples = bytes.size() / 3;
  std::vector<uint8_t> out(static_cast<size_t>(width) * 3, 0);
  if (samples == 0) {
    return out;
  }
  for (int x = 0; x < width; ++x) {
    const size_t i = samples * static_cast<size_t>(x) / static_cast<size_t>(width);
    const size_t idx = i * 3;
    if (idx + 2 >= bytes.size()) {
      break;
    }
    out[static_cast<size_t>(x) * 3] = bytes[idx];
    out[static_cast<size_t>(x) * 3 + 1] = bytes[idx + 1];
    out[static_cast<size_t>(x) * 3 + 2] = bytes[idx + 2];
  }
  return out;
}

inline bool DistinctStyles(const std::vector<uint8_t> &mood) {
  return Stripe(mood, MoodbarSettings::Style::Normal, kWidth) != Stripe(mood, MoodbarSettings::Style::Angry, kWidth);
}

}  // namespace MoodbarPreview

#endif
