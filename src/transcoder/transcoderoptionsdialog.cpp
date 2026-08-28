#include "transcoder/transcoderoptionsdialog.h"
#include "dialogs/dialogchrome.h"

#include "transcoder/transcoderoptionsaac.h"
#include "transcoder/transcoderoptionsasf.h"
#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsflac.h"
#include "transcoder/transcoderoptionslabels.h"
#include "transcoder/transcoderoptionsmp3.h"
#include "transcoder/transcoderoptionsopus.h"
#include "transcoder/transcoderoptionsplain.h"
#include "transcoder/transcoderoptionsspeex.h"
#include "transcoder/transcoderoptionsvorbis.h"
#include "transcoder/transcoderoptionswavpack.h"
#include "settings/settingswheelthrough.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <algorithm>
#include <string>

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
    case Transcoder::Format::WAV:
      return std::make_unique<TranscoderOptionsPlain>(Transcoder::Format::WAV, "wavenc");
    case Transcoder::Format::OggFlac:
      return std::make_unique<TranscoderOptionsOggFlac>();
    case Transcoder::Format::ALAC:
      return std::make_unique<TranscoderOptionsPlain>(Transcoder::Format::ALAC, "avenc_alac", "mp4mux");
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
  SettingsWheelThrough::Attach(spin);
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
  adw_dialog_set_title(dialog, Translations::CStr(TranscoderOptionsLabels::Title()));
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
  GtkWidget *managed = nullptr;
  GtkWidget *min_bitrate = nullptr;
  GtkWidget *max_bitrate = nullptr;
  GtkWidget *profile = nullptr;
  GtkWidget *tns = nullptr;
  GtkWidget *midside = nullptr;
  GtkWidget *shortctl = nullptr;
  GtkWidget *mode = nullptr;
  GtkWidget *vbr = nullptr;
  GtkWidget *abr = nullptr;
  GtkWidget *vad = nullptr;
  GtkWidget *dtx = nullptr;
  GtkWidget *complexity = nullptr;
  GtkWidget *nframes = nullptr;
  int default_quality = 5;

  if (format == Transcoder::Format::MP3) {
    TranscoderOptionsFields::Mp3 mp3;
    mp3.Load();
    default_quality = mp3.quality;
    static const char *targets[] = {TranscoderOptionsLabels::Quality(), TranscoderOptionsLabels::OptimizeBitrate(), nullptr};
    target_drop = gtk_drop_down_new_from_strings(targets);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(target_drop), static_cast<guint>(std::clamp(mp3.target, 0, 1)));
    gtk_box_append(GTK_BOX(box), target_drop);
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::Quality()), 0, 9, mp3.quality, &quality_spin));
    gtk_box_append(GTK_BOX(box),
                   LabeledSpin((std::string(TranscoderOptionsLabels::Bitrate()) + TranscoderOptionsLabels::Kbps()).c_str(), 32, 320,
                               mp3.bitrate, &bitrate_spin));
    static const char *engines[] = {TranscoderOptionsLabels::Fast(), TranscoderOptionsLabels::Standard(), TranscoderOptionsLabels::High(),
                                    nullptr};
    engine = gtk_drop_down_new_from_strings(engines);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(engine), static_cast<guint>(std::clamp(mp3.engine_quality, 0, 2)));
    gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(TranscoderOptionsLabels::EngineQuality())));
    gtk_box_append(GTK_BOX(box), engine);
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::ConstantBitrate()), mp3.cbr, &cbr));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::ForceMono()), mp3.mono, &mono));
  } else if (format == Transcoder::Format::FLAC || format == Transcoder::Format::OggFlac) {
    TranscoderOptionsFields::QualityEncoder flac;
    flac.max_quality = 8;
    flac.Load(format);
    default_quality = flac.quality;
    gtk_box_append(GTK_BOX(box), gtk_label_new((std::string(TranscoderOptionsLabels::Fast()) + " – " + TranscoderOptionsLabels::Best()).c_str()));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Compression (0–8)"), 0, 8, flac.quality, &quality_spin));
  } else if (format == Transcoder::Format::OggVorbis) {
    TranscoderOptionsFields::Vorbis vorbis;
    vorbis.Load();
    default_quality = vorbis.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Quality (0–10)"), 0, 10, vorbis.quality, &quality_spin));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Managed()), vorbis.managed, &managed));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::TargetBitrate()), 0, 500,
                                            vorbis.bitrate_bps > 0 ? vorbis.bitrate_bps / 1000 : 0, &bitrate_spin));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::MinBitrate()), 0, 500,
                                            vorbis.min_bitrate_bps > 0 ? vorbis.min_bitrate_bps / 1000 : 0, &min_bitrate));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::MaxBitrate()), 0, 500,
                                            vorbis.max_bitrate_bps > 0 ? vorbis.max_bitrate_bps / 1000 : 0, &max_bitrate));
  } else if (format == Transcoder::Format::Speex) {
    TranscoderOptionsFields::Speex speex;
    speex.Load();
    default_quality = speex.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Quality (0–10)"), 0, 10, speex.quality, &quality_spin));
    gtk_box_append(GTK_BOX(box), LabeledSpin((std::string(TranscoderOptionsLabels::Bitrate()) + TranscoderOptionsLabels::Kbps()).c_str(), 0,
                                            256, speex.bitrate_bps / 1000, &bitrate_spin));
    static const char *modes[] = {TranscoderOptionsLabels::Auto(), TranscoderOptionsLabels::Uwb(), TranscoderOptionsLabels::Wb(),
                                  TranscoderOptionsLabels::Nb(), nullptr};
    mode = gtk_drop_down_new_from_strings(modes);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(mode), static_cast<guint>(std::clamp(speex.mode, 0, 3)));
    gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(TranscoderOptionsLabels::EncodingMode())));
    gtk_box_append(GTK_BOX(box), mode);
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Vbr()), speex.vbr, &vbr));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::AverageBitrate()), 0, 256, speex.abr_bps / 1000, &abr));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Vad()), speex.vad, &vad));
    gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Dtx()), speex.dtx, &dtx));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::Complexity()), 0, 10, speex.complexity, &complexity));
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr(TranscoderOptionsLabels::Nframes()), 1, 10, speex.nframes, &nframes));
  } else if (format == Transcoder::Format::WavPack) {
    TranscoderOptionsFields::QualityEncoder encoder;
    encoder.Load(format);
    default_quality = encoder.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Mode (0 fast … 10 extra)"), 0, 10, encoder.quality, &quality_spin));
  } else if (format == Transcoder::Format::Opus) {
    TranscoderOptionsFields::Opus opus;
    opus.Load();
    default_quality = opus.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin((std::string(TranscoderOptionsLabels::Bitrate()) + TranscoderOptionsLabels::Kbps()).c_str(), 6, 510,
                                            std::max(6, opus.bitrate_bps / 1000), &bitrate_spin));
  } else if (format == Transcoder::Format::ASF) {
    TranscoderOptionsFields::Asf asf;
    asf.Load();
    default_quality = asf.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin((std::string(TranscoderOptionsLabels::Bitrate()) + TranscoderOptionsLabels::Kbps()).c_str(), 0, 320,
                                            std::max(0, asf.bitrate_bps / 1000), &bitrate_spin));
  } else {
    TranscoderOptionsFields::BitrateEncoder encoder;
    encoder.Load(format);
    default_quality = encoder.quality;
    gtk_box_append(GTK_BOX(box), LabeledSpin(Translations::CStr("Quality (0–10)"), 0, 10, encoder.quality, &quality_spin));
    if (format == Transcoder::Format::AAC) {
      TranscoderOptionsFields::Aac aac;
      aac.Load();
      aac.ApplyQuality(default_quality);
      gtk_box_append(GTK_BOX(box), LabeledSpin((std::string(TranscoderOptionsLabels::Bitrate()) + TranscoderOptionsLabels::Kbps()).c_str(),
                                              64, 320, aac.bitrate_bps / 1000, &bitrate_spin));
      static const char *profiles[] = {TranscoderOptionsLabels::Main(), TranscoderOptionsLabels::Lc(), TranscoderOptionsLabels::Ssr(),
                                       TranscoderOptionsLabels::Ltp(), nullptr};
      profile = gtk_drop_down_new_from_strings(profiles);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(profile), static_cast<guint>(std::clamp(aac.profile - 1, 0, 3)));
      gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(TranscoderOptionsLabels::Profile())));
      gtk_box_append(GTK_BOX(box), profile);
      gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Tns()), aac.tns, &tns));
      gtk_box_append(GTK_BOX(box), LabeledSwitch(Translations::CStr(TranscoderOptionsLabels::Midside()), aac.midside, &midside));
      static const char *blocks[] = {TranscoderOptionsLabels::NormalBlock(), TranscoderOptionsLabels::NoShort(),
                                     TranscoderOptionsLabels::NoLong(), nullptr};
      shortctl = gtk_drop_down_new_from_strings(blocks);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(shortctl), static_cast<guint>(std::clamp(aac.shortctl, 0, 2)));
      gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(TranscoderOptionsLabels::BlockType())));
      gtk_box_append(GTK_BOX(box), shortctl);
    } else {
      gtk_box_append(GTK_BOX(box), gtk_label_new((Translations::Tr("Bitrate ≈ ") + std::to_string(encoder.Bitrate()) + " kbps").c_str()));
    }
  }

  GtkWidget *pipeline = gtk_label_new(options->PipelineFragment().c_str());
  gtk_label_set_wrap(GTK_LABEL(pipeline), TRUE);
  gtk_widget_add_css_class(pipeline, "dim-label");
  gtk_box_append(GTK_BOX(box), pipeline);

  GtkWidget *apply = gtk_button_new_with_label(Translations::CStr("Apply"));
  gtk_widget_add_css_class(apply, "suggested-action");
  gtk_box_append(GTK_BOX(box), apply);
  DialogChrome::SetContent(dialog, box);

  g_object_set_data(G_OBJECT(apply), "format", GINT_TO_POINTER(static_cast<int>(format) + 1));
  g_object_set_data(G_OBJECT(apply), "quality", quality_spin);
  g_object_set_data(G_OBJECT(apply), "bitrate", bitrate_spin);
  g_object_set_data(G_OBJECT(apply), "target", target_drop);
  g_object_set_data(G_OBJECT(apply), "cbr", cbr);
  g_object_set_data(G_OBJECT(apply), "mono", mono);
  g_object_set_data(G_OBJECT(apply), "engine", engine);
  g_object_set_data(G_OBJECT(apply), "managed", managed);
  g_object_set_data(G_OBJECT(apply), "min-bitrate", min_bitrate);
  g_object_set_data(G_OBJECT(apply), "max-bitrate", max_bitrate);
  g_object_set_data(G_OBJECT(apply), "profile", profile);
  g_object_set_data(G_OBJECT(apply), "tns", tns);
  g_object_set_data(G_OBJECT(apply), "midside", midside);
  g_object_set_data(G_OBJECT(apply), "shortctl", shortctl);
  g_object_set_data(G_OBJECT(apply), "mode", mode);
  g_object_set_data(G_OBJECT(apply), "vbr", vbr);
  g_object_set_data(G_OBJECT(apply), "abr", abr);
  g_object_set_data(G_OBJECT(apply), "vad", vad);
  g_object_set_data(G_OBJECT(apply), "dtx", dtx);
  g_object_set_data(G_OBJECT(apply), "complexity", complexity);
  g_object_set_data(G_OBJECT(apply), "nframes", nframes);
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
                       auto *engine_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "engine"));
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
                         mp3.engine_quality = static_cast<int>(gtk_drop_down_get_selected(engine_w));
                       }
                       if (cbr_w) {
                         mp3.cbr = gtk_switch_get_active(cbr_w) == TRUE;
                       }
                       if (mono_w) {
                         mp3.mono = gtk_switch_get_active(mono_w) == TRUE;
                       }
                       mp3.Save();
                     } else if (format == Transcoder::Format::OggVorbis) {
                       TranscoderOptionsFields::Vorbis vorbis;
                       vorbis.Load();
                       vorbis.ApplyQuality(quality);
                       if (auto *managed_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "managed"))) {
                         vorbis.managed = gtk_switch_get_active(managed_w) == TRUE;
                       }
                       auto kbps = [](gpointer widget) {
                         return widget ? static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget))) * 1000 : 0;
                       };
                       const int bitrate = kbps(g_object_get_data(G_OBJECT(button), "bitrate"));
                       vorbis.bitrate_bps = bitrate > 0 ? bitrate : -1;
                       const int min_rate = kbps(g_object_get_data(G_OBJECT(button), "min-bitrate"));
                       vorbis.min_bitrate_bps = min_rate > 0 ? min_rate : -1;
                       const int max_rate = kbps(g_object_get_data(G_OBJECT(button), "max-bitrate"));
                       vorbis.max_bitrate_bps = max_rate > 0 ? max_rate : -1;
                       vorbis.Save();
                     } else if (format == Transcoder::Format::Speex) {
                       TranscoderOptionsFields::Speex speex;
                       speex.Load();
                       speex.ApplyQuality(quality);
                       if (auto *bitrate_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "bitrate"))) {
                         speex.bitrate_bps = static_cast<int>(gtk_spin_button_get_value(bitrate_w)) * 1000;
                       }
                       if (auto *mode_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "mode"))) {
                         speex.mode = static_cast<int>(gtk_drop_down_get_selected(mode_w));
                       }
                       if (auto *vbr_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "vbr"))) {
                         speex.vbr = gtk_switch_get_active(vbr_w) == TRUE;
                       }
                       if (auto *abr_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "abr"))) {
                         speex.abr_bps = static_cast<int>(gtk_spin_button_get_value(abr_w)) * 1000;
                       }
                       if (auto *vad_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "vad"))) {
                         speex.vad = gtk_switch_get_active(vad_w) == TRUE;
                       }
                       if (auto *dtx_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "dtx"))) {
                         speex.dtx = gtk_switch_get_active(dtx_w) == TRUE;
                       }
                       if (auto *complexity_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "complexity"))) {
                         speex.complexity = static_cast<int>(gtk_spin_button_get_value(complexity_w));
                       }
                       if (auto *nframes_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "nframes"))) {
                         speex.nframes = static_cast<int>(gtk_spin_button_get_value(nframes_w));
                       }
                       speex.Save();
                     } else if (format == Transcoder::Format::AAC) {
                       TranscoderOptionsFields::Aac aac;
                       aac.Load();
                       aac.ApplyQuality(quality);
                       if (auto *bitrate_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "bitrate"))) {
                         aac.bitrate_bps = static_cast<int>(gtk_spin_button_get_value(bitrate_w)) * 1000;
                       }
                       if (auto *profile_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "profile"))) {
                         aac.profile = static_cast<int>(gtk_drop_down_get_selected(profile_w)) + 1;
                       }
                       if (auto *tns_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "tns"))) {
                         aac.tns = gtk_switch_get_active(tns_w) == TRUE;
                       }
                       if (auto *midside_w = GTK_SWITCH(g_object_get_data(G_OBJECT(button), "midside"))) {
                         aac.midside = gtk_switch_get_active(midside_w) == TRUE;
                       }
                       if (auto *short_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "shortctl"))) {
                         aac.shortctl = static_cast<int>(gtk_drop_down_get_selected(short_w));
                       }
                       aac.Save();
                     } else if (format == Transcoder::Format::Opus) {
                       TranscoderOptionsFields::Opus opus;
                       opus.Load();
                       if (auto *bitrate_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "bitrate"))) {
                         opus.bitrate_bps = static_cast<int>(gtk_spin_button_get_value(bitrate_w)) * 1000;
                         quality = std::clamp((opus.bitrate_bps / 1000 - 48) * 10 / 208, 0, 10);
                         opus.quality = quality;
                       } else {
                         opus.ApplyQuality(quality);
                       }
                       opus.Save();
                     } else if (format == Transcoder::Format::ASF) {
                       TranscoderOptionsFields::Asf asf;
                       asf.Load();
                       if (auto *bitrate_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "bitrate"))) {
                         asf.bitrate_bps = static_cast<int>(gtk_spin_button_get_value(bitrate_w)) * 1000;
                         quality = std::clamp((asf.bitrate_bps / 1000 - 64) * 10 / 128, 0, 10);
                         asf.quality = quality;
                       } else {
                         asf.ApplyQuality(quality);
                       }
                       asf.Save();
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
