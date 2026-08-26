#include "moodbar/moodbarrenderer.h"

#include "constants/moodbarsettings.h"
#include "core/settings.h"
#include "moodbar/moodbarstyle.h"

void MoodbarRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood) {
  if (mood.empty() || width <= 0 || height <= 0) {
    return;
  }
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  const MoodbarSettings::Style style =
      MoodbarStyle::ClampStyle(settings.IntValue(MoodbarSettings::kStyle, static_cast<int>(MoodbarSettings::kDefaultStyle)));
  const std::vector<uint8_t> styled = MoodbarStyle::Apply(mood, style);
  const std::vector<uint8_t> &bytes = styled.empty() ? mood : styled;
  const size_t samples = bytes.size() / 3;
  if (samples == 0) {
    return;
  }
  for (int x = 0; x < width; ++x) {
    const size_t i = samples * static_cast<size_t>(x) / static_cast<size_t>(width);
    const size_t idx = i * 3;
    if (idx + 2 >= bytes.size()) {
      break;
    }
    cairo_set_source_rgb(cr, bytes[idx] / 255.0, bytes[idx + 1] / 255.0, bytes[idx + 2] / 255.0);
    cairo_move_to(cr, x + 0.5, 0);
    cairo_line_to(cr, x + 0.5, height);
    cairo_stroke(cr);
  }
}
