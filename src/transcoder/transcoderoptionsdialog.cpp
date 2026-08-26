#include "transcoder/transcoderoptionsdialog.h"

#include "transcoder/transcoderoptionsaac.h"
#include "transcoder/transcoderoptionsasf.h"
#include "transcoder/transcoderoptionsflac.h"
#include "transcoder/transcoderoptionsmp3.h"
#include "transcoder/transcoderoptionsopus.h"
#include "transcoder/transcoderoptionsspeex.h"
#include "transcoder/transcoderoptionsvorbis.h"
#include "transcoder/transcoderoptionswavpack.h"

#include <adwaita.h>

std::unique_ptr<TranscoderOptionsInterface> TranscoderOptionsDialog::OptionsFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::MP3:
      return std::make_unique<TranscoderOptionsMp3>();
    case Transcoder::Format::AAC:
      return std::make_unique<TranscoderOptionsAac>();
    case Transcoder::Format::FLAC:
      return std::make_unique<TranscoderOptionsFlac>();
    case Transcoder::Format::OggVorbis:
      return std::make_unique<TranscoderOptionsVorbis>();
    case Transcoder::Format::Opus:
      return std::make_unique<TranscoderOptionsOpus>();
    case Transcoder::Format::Speex:
      return std::make_unique<TranscoderOptionsSpeex>();
    case Transcoder::Format::WavPack:
      return std::make_unique<TranscoderOptionsWavPack>();
    case Transcoder::Format::ASF:
      return std::make_unique<TranscoderOptionsAsf>();
  }
  return std::make_unique<TranscoderOptionsFlac>();
}

void TranscoderOptionsDialog::Show(GtkWindow *parent, Transcoder::Format format, const std::function<void(int quality)> &applied) {
  auto options = OptionsFor(format);
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new((Transcoder::FormatName(format) + " options").c_str(),
                                                                ("Encoder: " + options->PipelineFragment()).c_str()));
  GtkWidget *quality = gtk_spin_button_new_with_range(0, 10, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(quality), 5);
  adw_alert_dialog_set_extra_child(dialog, quality);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "apply", "Apply", nullptr);
  auto *fn = new std::function<void(int)>(applied);
  g_object_set_data_full(G_OBJECT(dialog), "applied", fn, [](gpointer p) { delete static_cast<std::function<void(int)> *>(p); });
  g_object_set_data(G_OBJECT(dialog), "quality", quality);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer) {
                     if (g_strcmp0(response, "apply") != 0) {
                       return;
                     }
                     auto *cb = static_cast<std::function<void(int)> *>(g_object_get_data(G_OBJECT(alert), "applied"));
                     auto *spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(alert), "quality"));
                     if (cb && spin) {
                       (*cb)(static_cast<int>(gtk_spin_button_get_value(spin)));
                     }
                   }),
                   nullptr);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
