#ifndef STRAWBERRY_ANALYZERBASE_H
#define STRAWBERRY_ANALYZERBASE_H

#include <cairo.h>

#include <string>
#include <vector>

class AnalyzerBase {
 public:
  virtual ~AnalyzerBase() = default;
  virtual std::string name() const = 0;
  virtual void Draw(cairo_t *cr, int width, int height, const std::vector<float> &bands) const = 0;
  virtual void Advance(int width, int height, const std::vector<float> &bands) { (void)width; (void)height; (void)bands; }

 protected:
  static double BarWidth(int width, const std::vector<float> &bands);
  static void DrawWave(cairo_t *cr, int width, int height, const std::vector<float> &bands, double r, double g, double b);
  static void DrawBars(cairo_t *cr, int width, int height, const std::vector<float> &bands, bool rainbow, bool turbine, bool blocks);
};

#endif
