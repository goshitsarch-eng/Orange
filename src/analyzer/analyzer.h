#ifndef STRAWBERRY_ANALYZER_H
#define STRAWBERRY_ANALYZER_H

#include "analyzer/analyzercontainer.h"
#include "analyzer/analyzerdemo.h"
#include "analyzer/fht.h"

#include <cairo.h>
#include <glib.h>

#include <cstdint>
#include <string>
#include <vector>

class Analyzer {
 public:
  Analyzer();
  void ReloadSettings();
  void Save() const;
  void SetEngineScope(const std::vector<int16_t> &scope);
  void SetMagnitudes(const std::vector<float> &db);
  void set_paused(bool paused) { paused_ = paused; }
  bool paused() const { return paused_; }
  void set_playing(bool playing) { playing_ = playing; }
  bool playing() const { return playing_; }
  const std::vector<float> &bands() const { return bands_; }
  void set_type(const std::string &type);
  const std::string &type() const { return type_; }
  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled);
  int framerate() const { return framerate_; }
  void set_framerate(int fps);
  void Draw(cairo_t *cr, int width, int height);
  static std::vector<std::string> Types();
  static int ClampFramerate(int fps);
  static std::string NextType(const std::string &current);

 private:
  void MaybeAdvance(int width, int height, const std::vector<float> &bands);

  std::vector<float> bands_ = std::vector<float>(32, 0.0f);
  std::string type_ = "Bar";
  bool enabled_ = true;
  int framerate_ = 25;
  FHT fht_{64};
  AnalyzerContainer container_;
  AnalyzerDemo demo_;
  bool paused_ = false;
  bool playing_ = false;
  int last_width_ = 0;
  int last_height_ = 0;
  gint64 last_advance_us_ = 0;
};

#endif
