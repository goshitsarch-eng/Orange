#ifndef STRAWBERRY_PLAYLISTDROPINDICATOR_H
#define STRAWBERRY_PLAYLISTDROPINDICATOR_H

#include <gtk/gtk.h>

#include <string>


namespace PlaylistDropIndicator {

inline constexpr int kLineWidth = 2;
inline constexpr int kGradientWidth = 5;

enum class Position { Above, Below, Empty };

struct State {
  int insert_row = -1;
  Position pos = Position::Empty;
  int line_y = -1;
};

inline State FromPointer(const double y, const int row_index, const double row_y, const double row_height, const bool has_rows,
                         const double last_bottom) {
  if (!has_rows) {
    return {0, Position::Empty, 1};
  }
  if (row_index < 0) {
    return {0, Position::Empty, static_cast<int>(last_bottom)};
  }
  if (y < row_y + row_height / 2.0) {
    return {row_index, Position::Above, static_cast<int>(row_y)};
  }
  return {row_index + 1, Position::Below, static_cast<int>(row_y + row_height)};
}

inline bool Active(const State &state) { return state.line_y >= 0 && state.insert_row >= 0; }

inline int InsertRow(const State &state, const int fallback) { return Active(state) ? state.insert_row : fallback; }

// The class whose CSS gives the indicator the theme's accent colour, so the drawing code can read it back
// with gtk_widget_get_color() instead of painting a fixed blue that ignores the theme entirely.
inline constexpr const char *kCssClass = "playlist-drop-indicator";

inline std::string Css() { return std::string(".") + kCssClass + " { color: @accent_bg_color; }"; }

inline void Draw(GtkWidget *area, cairo_t *cr, int width, const State &state) {
  if (!Active(state)) {
    return;
  }
  GdkRGBA accent;
  gtk_widget_get_color(area, &accent);
  const double y = state.line_y;
  cairo_pattern_t *grad = cairo_pattern_create_linear(0, y - kGradientWidth, 0, y + kGradientWidth);
  cairo_pattern_add_color_stop_rgba(grad, 0.0, accent.red, accent.green, accent.blue, 0.0);
  cairo_pattern_add_color_stop_rgba(grad, 0.5, accent.red, accent.green, accent.blue, 0.35 * accent.alpha);
  cairo_pattern_add_color_stop_rgba(grad, 1.0, accent.red, accent.green, accent.blue, 0.0);
  cairo_set_source(cr, grad);
  cairo_rectangle(cr, 0, y - kGradientWidth, width, kGradientWidth * 2);
  cairo_fill(cr);
  cairo_pattern_destroy(grad);
  cairo_set_source_rgba(cr, accent.red, accent.green, accent.blue, accent.alpha);
  cairo_rectangle(cr, 0, y, width, kLineWidth);
  cairo_fill(cr);
}

}  // namespace PlaylistDropIndicator

#endif
