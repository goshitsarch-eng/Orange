#ifndef STRAWBERRY_TRANSCODEROPTIONSDIALOG_H
#define STRAWBERRY_TRANSCODEROPTIONSDIALOG_H

#include "transcoder/transcoder.h"

#include <functional>
#include <gtk/gtk.h>
#include <memory>

class TranscoderOptionsInterface;

class TranscoderOptionsDialog {
 public:
  static std::unique_ptr<TranscoderOptionsInterface> OptionsFor(Transcoder::Format format);
  static void Show(GtkWindow *parent, Transcoder::Format format, const std::function<void(int quality)> &applied);
};

#endif
