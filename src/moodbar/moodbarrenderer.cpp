#include "moodbar/moodbarrenderer.h"

#include "constants/moodbarsettings.h"
#include "core/settings.h"
#include "moodbar/moodbarplayhead.h"
#include "moodbar/moodbarstyle.h"

namespace {

void DrawStripes(cairo_t *cr, int width, int height, const std::vector<uint8_t> &bytes) {
  const size_t samples = bytes.size() / 3;
  if (samples == 0 || width <= 0 || height <= 0) {
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

void DrawArrow(cairo_t *cr, int left, double fill_r, double fill_g, double fill_b) {
  const double right = left + MoodbarPlayhead::kArrowWidth;
  const double bottom = MoodbarPlayhead::kArrowHeight;
  const double mid = left + MoodbarPlayhead::kArrowWidth / 2.0;
  cairo_save(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_move_to(cr, left + 0.5, 0.5);
  cairo_line_to(cr, right - 0.5, 0.5);
  cairo_line_to(cr, mid, bottom - 0.5);
  cairo_close_path(cr);
  cairo_set_source_rgb(cr, fill_r, fill_g, fill_b);
  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
  cairo_restore(cr);
}

}  // namespace

void MoodbarRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood, int64_t position_nanosec,
                           int64_t length_nanosec, double fill_r, double fill_g, double fill_b) {
  if (mood.empty() || width <= 0 || height <= 0) {
    return;
  }
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  const MoodbarSettings::Style style =
      MoodbarStyle::ClampStyle(settings.IntValue(MoodbarSettings::kStyle, static_cast<int>(MoodbarSettings::kDefaultStyle)));
  const std::vector<uint8_t> styled = MoodbarStyle::Apply(mood, style);
  const std::vector<uint8_t> &bytes = styled.empty() ? mood : styled;
  const bool playhead = MoodbarPlayhead::ShowPlayheadFromPosition(position_nanosec);
  if (!playhead) {
    DrawStripes(cr, width, height, bytes);
    return;
  }
  const int inner_w = MoodbarPlayhead::InnerWidth(width);
  const int inner_h = MoodbarPlayhead::InnerHeight(height);
  if (inner_w > 0 && inner_h > 0) {
    cairo_save(cr);
    cairo_translate(cr, MoodbarPlayhead::kMarginSize + MoodbarPlayhead::kBorderSize,
                    MoodbarPlayhead::kMarginSize + MoodbarPlayhead::kBorderSize);
    DrawStripes(cr, inner_w, inner_h, bytes);
    cairo_restore(cr);
  }
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_set_line_width(cr, MoodbarPlayhead::kBorderSize);
  cairo_rectangle(cr, MoodbarPlayhead::kMarginSize + 0.5, MoodbarPlayhead::kMarginSize + 0.5,
                  width - 2 * MoodbarPlayhead::kMarginSize - 1, height - 2 * MoodbarPlayhead::kMarginSize - 1);
  cairo_stroke(cr);
  DrawArrow(cr, MoodbarPlayhead::ArrowLeft(position_nanosec, length_nanosec, width), fill_r, fill_g, fill_b);
}

