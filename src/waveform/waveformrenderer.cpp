#include "waveform/waveformrenderer.h"

#include "constants/waveformsettings.h"
#include "core/settings.h"
#include "waveform/waveformplayhead.h"
#include "waveform/waveformstyle.h"

void WaveformRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<float> &peaks, int64_t position_nanosec,
                            int64_t length_nanosec, double cursor_r, double cursor_g, double cursor_b) {
  if (peaks.empty() || width <= 0 || height <= 0) {
    return;
  }
  Settings settings;
  settings.BeginGroup(WaveformSettings::kSettingsGroup);
  const ColorUtils::Rgb color = WaveformStyle::BarColorFromHex(settings.Value(WaveformSettings::kColor, WaveformSettings::kDefaultColor));
  const bool playhead = WaveformPlayhead::ShowPlayheadFromPosition(position_nanosec);
  const int split = playhead ? WaveformPlayhead::SplitX(position_nanosec, length_nanosec, width) : 0;
  const double mid = height / 2.0;
  for (int x = 0; x < width; ++x) {
    const size_t i = peaks.size() * static_cast<size_t>(x) / static_cast<size_t>(width);
    const double h = WaveformStyle::ShapedAmplitude(peaks[i]) * mid;
    if (playhead && x < split) {
      cairo_set_source_rgba(cr, color.r / 255.0, color.g / 255.0, color.b / 255.0, WaveformPlayhead::kPlayedAlpha);
    } else {
      cairo_set_source_rgb(cr, color.r / 255.0, color.g / 255.0, color.b / 255.0);
    }
    cairo_move_to(cr, x + 0.5, mid - h);
    cairo_line_to(cr, x + 0.5, mid + h);
    cairo_stroke(cr);
  }
  if (!playhead) {
    return;
  }
  cairo_set_source_rgb(cr, cursor_r, cursor_g, cursor_b);
  cairo_set_line_width(cr, WaveformPlayhead::kCursorWidth);
  cairo_move_to(cr, split + 0.5, 0);
  cairo_line_to(cr, split + 0.5, height);
  cairo_stroke(cr);
}
