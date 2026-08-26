#include "moodbar/moodbarrenderer.h"

void MoodbarRenderer::Draw(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood) {
  if (mood.empty() || width <= 0 || height <= 0) {
    return;
  }
  const size_t samples = mood.size() / 3;
  for (int x = 0; x < width; ++x) {
    const size_t i = samples * static_cast<size_t>(x) / static_cast<size_t>(width);
    const size_t idx = i * 3;
    if (idx + 2 >= mood.size()) {
      break;
    }
    cairo_set_source_rgb(cr, mood[idx] / 255.0, mood[idx + 1] / 255.0, mood[idx + 2] / 255.0);
    cairo_move_to(cr, x + 0.5, 0);
    cairo_line_to(cr, x + 0.5, height);
    cairo_stroke(cr);
  }
}
