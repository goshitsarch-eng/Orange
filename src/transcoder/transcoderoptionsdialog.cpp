#include "transcoder/transcoderoptionsdialog.h"

#include "transcoder/transcoderoptionsaac.h"
#include "transcoder/transcoderoptionsasf.h"
#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsflac.h"
#include "transcoder/transcoderoptionsmp3.h"
#include "transcoder/transcoderoptionsopus.h"
#include "transcoder/transcoderoptionsspeex.h"
#include "transcoder/transcoderoptionsvorbis.h"
#include "transcoder/transcoderoptionswavpack.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <algorithm>

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

namespace {

GtkWidget *LabeledSpin(const char *label, double min, double max, double value, GtkWidget **spin_out) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *name = gtk_label_new(label);
  gtk_widget_set_hexpand(name, TRUE);
  gtk_widget_set_halign(name, GTK_ALIGN_START);
  GtkWidget *spin = gtk_spin_button_new_with_range(min, max, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);
  gtk_box_append(GTK_BOX(row), name);
  gtk_box_append(GTK_BOX(row), spin);
  *spin_out = spin;
  return row;
}

GtkWidget *LabeledSwitch(const char *label, bool on, GtkWidget **switch_out) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *name = gtk_label_new(label);
  gtk_widget_set_hexpand(name, TRUE);
  gtk_widget_set_halign(name, GTK_ALIGN_START);
  GtkWidget *toggle = gtk_switch_new();
  gtk_switch_set_active(GTK_SWITCH(toggle), on ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(row), name);
  gtk_box_append(GTK_BOX(row), toggle);
  *switch_out = toggle;
  return row;
}

}  // namespace

void TranscoderOptionsDialog::Show(GtkWindow *parent, Transcoder::Format format, const std::function<void(int quality)> &applied) {
  auto options = OptionsFor(format);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, (Transcoder::FormatName(format) + " " + Translations::Tr("options")).c_str());
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  GtkWidget *quality_spin = nullptr;
  GtkWidget *bitrate_spin = nullptr;
  GtkWidget *target_drop = nullptr;
  GtkWidget *cbr = nullptr;
  GtkWidget *mono = nullptr;
  GtkWidget *engine = nullptr;
  int default_quality = 5;

  if (format == Transcoder::Format::MP3) {
    TranscoderOptionsFields::Mp3 mp3;
    mp3.Load();
    default_quality = mp3.quality;
    static const char *targets[] = {"Quality (VBR)", "Bitrate", nullptr};
    target_drop = gtk_drop_down_new_from_strings(targets);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(target_drop), static_cast<guint>(std::clamp(mp3.target, 0, 1)));
    gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Encoding target")));
    gtk_box_append(GTK_BOX(box), target_drop);
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("VBR quality (0–9)"), 0, 9, mp3.quality, &quality_spin));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Bitrate (kbps)"), 32, 320, mp3.bitrate, &bitrate_spin));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Engine quality (0–2)"), 0, 2, mp3.engine_quality, &engine));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr("Constant bitrate"), mp3.cbr, &cbr));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr("Mono"), mp3.mono, &mono));
  } else if (format == Transcoder::Format::FLAC) {
    TranscoderOptionsFields::QualityEncoder flac;
    flac.max_quality = 8;
    flac.Load(format);
    default_quality = flac.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Compression (0–8)"), 0, 8, flac.quality, &quality_spin));
  } else if (format == Transcoder::Format::OggVorbis || format == Transcoder::Format::Speex) {
    TranscoderOptionsFields::QualityEncoder encoder;
    encoder.Load(format);
    default_quality = encoder.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Quality (0–10)"), 0, 10, encoder.quality, &quality_spin));
  } else if (format == Transcoder::Format::WavPack) {
    TranscoderOptionsFields::QualityEncoder encoder;
    encoder.Load(format);
    default_quality = encoder.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Mode (0 fast … 10 extra)"), 0, 10, encoder.quality, &quality_spin));
  } else {
    TranscoderOptionsFields::BitrateEncoder encoder;
    if (format == Transcoder::Format::Opus) {
      encoder.min_kbps = 48;
      encoder.max_kbps = 256;
    } else if (format == Transcoder::Format::ASF) {
      encoder.max_kbps = 192;
    }
    encoder.Load(format);
    default_quality = encoder.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Quality (0–10)"), 0, 10, encoder.quality, &quality_spin));
    gtk_box_append(GTK_BOX(box), gtk_label_new((Translations::Tr("Bitrate ≈ ") + std::to_string(encoder.Bitrate()) + " kbps").c_str()));
  }

  GtkWidget *pipeline = gtk_label_new(options->PipelineFragment().c_str());
  gtk_label_set_wrap(GTK_LABEL(pipeline), TRUE);
  gtk_widget_add_css_class(pipeline, "dim-label");
  gtk_box_append(GTK_BOX(box), pipeline);

  GtkWidget *apply = gtk_button_new_with_label(Translations::CStr("Apply"));
  gtk_widget_add_css_class(apply, "suggested-action");
  gtk_box_append(GTK_BOX(box), apply);
  adw_dialog_set_child(dialog, box);

  g_object_set_data(G_OBJECT(apply), "format", GINT_TO_POINTER(static_cast<int>(format) + 1));
  g_object_set_data(G_OBJECT(apply), "quality", quality_spin);
  g_object_set_data(G_OBJECT(apply), "bitrate", bitrate_spin);
  g_object_set_data(G_OBJECT(apply), "target", target_drop);
  g_object_set_data(G_OBJECT(apply), "cbr", cbr);
  g_object_set_data(G_OBJECT(apply), "mono", mono);
  g_object_set_data(G_OBJECT(apply), "engine", engine);
  auto *fn = new std::function<void(int)>(applied);
  g_object_set_data_full(G_OBJECT(apply), "applied", fn, [](gpointer p) { delete static_cast<std::function<void(int)> *>(p); });
  g_object_set_data(G_OBJECT(apply), "default-quality", GINT_TO_POINTER(default_quality + 1));

  g_signal_connect(apply, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     const auto format = static_cast<Transcoder::Format>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "format")) - 1);
                     auto *quality_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "quality"));
                     int quality = quality_w ? static_cast<int>(gtk_spin_button_get_value(quality_w))
                                             : (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "default-quality")) - 1);
                     if (format == Transcoder::Format::MP3) {
                       TranscoderOptionsFields::Mp3 mp3;
                       mp3.Load();
                       auto *target_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "target"));
                       auto *bitrate_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "bitrate"));
                       auto *engine_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "engine"));
                       auto *cbr_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "cbr"));
                       auto *mono_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "mono"));
                       if (target_w) {
                         mp3.target = static_cast<int>(gtk_drop_down_get_selected(target_w));
                       }
                       mp3.quality = quality;
                       if (bitrate_w) {
                         mp3.bitrate = static_cast<int>(gtk_spin_button_get_value(bitrate_w));
                       }
                       if (engine_w) {
                         mp3.engine_quality = static_cast<int>(gtk_spin_button_get_value(engine_w));
                       }
                       if (cbr_w) {
                         mp3.cbr = gtk_switch_get_active(cbr_w) == TRUE;
                       }
                       if (mono_w) {
                         mp3.mono = gtk_switch_get_active(mono_w) == TRUE;
                       }
                       mp3.Save();
                     } else if (format == Transcoder::Format::AAC || format == Transcoder::Format::Opus || format == Transcoder::Format::ASF) {
                       TranscoderOptionsFields::BitrateEncoder encoder;
                       encoder.ApplyQuality(quality);
                       encoder.Save(format);
                     } else {
                       TranscoderOptionsFields::QualityEncoder encoder;
                       encoder.ApplyQuality(quality);
                       encoder.Save(format);
                     }
                     auto *cb = static_cast<std::function<void(int)> *>(g_object_get_data(G_OBJECT(button), "applied"));
                     if (cb) {
                       (*cb)(quality);
                     }
                     adw_dialog_close(ADW_DIALOG(data));
                   })),
                   dialog);

  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
