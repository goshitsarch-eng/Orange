#include "waveform/waveformrenderer.h"

#include "constants/waveformsettings.h"
#include "core/settings.h"
#include "waveform/waveformstyle.h"

void WaveformRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &peaks) {
  if (peaks.empty() || width <= 0 || height <= 0) {
    return;
  }
  Settings settings;
  settings.BeginGroup(WaveformSettings::kSettingsGroup);
  const ColorUtils::Rgb color = WaveformStyle::BarColorFromHex(settings.Value(WaveformSettings::kColor, WaveformSettings::kDefaultColor));
  cairo_set_source_rgb(cr, color.r / 255.0, color.g / 255.0, color.b / 255.0);
  const double mid = height / 2.0;
  for (int x = 0; x < width; ++x) {
    const size_t i = peaks.size() * static_cast<size_t>(x) / static_cast<size_t>(width);
    const double h = WaveformStyle::ShapedAmplitude(peaks[i]) * mid;
    cairo_move_to(cr, x + 0.5, mid - h);
    cairo_line_to(cr, x + 0.5, mid + h);
    cairo_stroke(cr);
  }
}
