#include "equalizer/equalizerdialog.h"
#include "dialogs/dialogchrome.h"

#include "core/application.h"
#include "core/player.h"
#include "equalizer/equalizer.h"
#include "equalizer/equalizercontrols.h"
#include "equalizer/equalizergain.h"
#include "equalizer/equalizerlabels.h"
#include "equalizer/equalizerpersist.h"
#include "dialogs/dialogclosekeys.h"
#include "equalizer/equalizerpresets.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <functional>
#include <string>
#include <vector>

namespace {

struct EqualizerDeleteJob {
  Equalizer *eq = nullptr;
  GtkDropDown *drop = nullptr;
};

struct EqualizerSaveJob {
  Equalizer *eq = nullptr;
  std::function<void()> after;
};

void PromptSaveIfDirty(GtkWidget *parent, Equalizer *eq, const std::string &name, std::function<void()> after) {
  if (!eq || !EqualizerPersist::ShouldPromptSave(eq->HasPreset(name), eq->MatchesPreset(name))) {
    if (after) {
      after();
    }
    return;
  }
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(EqualizerLabels::SavePreset()), Translations::CStr("Name")));
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), name.c_str());
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", Translations::CStr("Cancel"), "save", Translations::CStr(EqualizerLabels::SavePreset()),
                                 nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response(dialog, "save");
  adw_alert_dialog_set_close_response(dialog, "cancel");
  auto *job = new EqualizerSaveJob{eq, std::move(after)};
  g_object_set_data_full(G_OBJECT(dialog), "save-job", job, [](gpointer p) { delete static_cast<EqualizerSaveJob *>(p); });
  g_object_set_data(G_OBJECT(dialog), "save-entry", entry);
  g_signal_connect(dialog, "response", G_CALLBACK((+[](AdwAlertDialog *alert, const char *response, gpointer) {
                     auto *pending = static_cast<EqualizerSaveJob *>(g_object_get_data(G_OBJECT(alert), "save-job"));
                     GtkWidget *field = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "save-entry"));
                     if (pending && pending->eq && g_strcmp0(response, "save") == 0 && field) {
                       const char *text = gtk_editable_get_text(GTK_EDITABLE(field));
                       const std::string chosen = text ? text : "";
                       if (EqualizerPresets::CanSave(chosen, pending->eq->IsBuiltin(chosen))) {
                         pending->eq->SavePreset(chosen);
                       }
                     }
                     if (pending && pending->after) {
                       pending->after();
                     }
                   })),
                   nullptr);
  adw_dialog_present(ADW_DIALOG(dialog), parent);
}

void ApplyEqualizerDelete(Equalizer *eq, GtkDropDown *drop, const std::string &name) {
  if (!eq || !drop || name.empty() || eq->IsBuiltin(name)) {
    return;
  }
  const std::vector<std::string> remaining = EqualizerPresets::AfterDelete(eq->Presets(), name);
  if (!eq->DeletePreset(name)) {
    return;
  }
  const std::string next = EqualizerPresets::NextSelected(remaining, name, eq->selected_preset());
  GtkStringList *list = GTK_STRING_LIST(gtk_drop_down_get_model(drop));
  while (g_list_model_get_n_items(G_LIST_MODEL(list)) > 0) {
    gtk_string_list_remove(list, 0);
  }
  for (const std::string &preset : eq->Presets()) {
    gtk_string_list_append(list, preset.c_str());
  }
  eq->LoadPreset(next);
  gtk_drop_down_set_selected(drop, static_cast<guint>(EqualizerPresets::IndexOf(eq->Presets(), next)));
}

}  // namespace

void EqualizerDialog::Show(GtkWindow *parent, Equalizer *equalizer, Application *app) {
  if (!equalizer) {
    return;
  }
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Equalizer"));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *enable = gtk_check_button_new_with_label(Translations::CStr(EqualizerLabels::Enable()));
  gtk_widget_set_tooltip_text(enable, Translations::CStr(EqualizerLabels::RestartHint()));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(enable), equalizer->enabled());
  gtk_box_append(GTK_BOX(box), enable);
  GtkWidget *enable_hint = gtk_label_new(Translations::CStr(EqualizerLabels::RestartHint()));
  gtk_label_set_wrap(GTK_LABEL(enable_hint), TRUE);
  gtk_widget_set_halign(enable_hint, GTK_ALIGN_START);
  gtk_widget_add_css_class(enable_hint, "dim-label");
  gtk_box_append(GTK_BOX(box), enable_hint);
  GtkStringList *preset_names = gtk_string_list_new(nullptr);
  guint selected_preset = 0;
  guint preset_index = 0;
  for (const std::string &name : equalizer->Presets()) {
    gtk_string_list_append(preset_names, name.c_str());
    if (name == equalizer->selected_preset()) {
      selected_preset = preset_index;
    }
    ++preset_index;
  }
  GtkWidget *preset = gtk_drop_down_new(G_LIST_MODEL(preset_names), nullptr);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(preset), selected_preset);
  g_object_set_data_full(G_OBJECT(preset), "last-preset", g_strdup(equalizer->selected_preset().c_str()), g_free);
  g_signal_connect(preset, "notify::selected",
                   G_CALLBACK((+[](GtkDropDown *drop, GParamSpec *, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     const char *last = static_cast<const char *>(g_object_get_data(G_OBJECT(drop), "last-preset"));
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     const std::string next = item ? gtk_string_object_get_string(item) : "";
                     auto load = [eq, drop, next]() {
                       if (eq && !next.empty()) {
                         eq->LoadPreset(next);
                       }
                       if (drop) {
                         g_object_set_data_full(G_OBJECT(drop), "last-preset", g_strdup(next.c_str()), g_free);
                       }
                     };
                     PromptSaveIfDirty(GTK_WIDGET(drop), eq, last ? last : "", load);
                   })),
                   equalizer);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(EqualizerLabels::Preset())));
  gtk_box_append(GTK_BOX(box), preset);
  GtkWidget *preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *preset_name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(preset_name), "Custom preset name");
  GtkWidget *save_preset = gtk_button_new_with_label(Translations::CStr(EqualizerLabels::SavePreset()));
  GtkWidget *delete_preset = gtk_button_new_with_label(Translations::CStr(EqualizerLabels::DeletePreset()));
  gtk_widget_set_sensitive(save_preset, FALSE);
  g_object_set_data(G_OBJECT(save_preset), "name", preset_name);
  g_object_set_data(G_OBJECT(save_preset), "list", preset_names);
  g_object_set_data(G_OBJECT(preset_name), "save", save_preset);
  g_signal_connect(preset_name, "changed", G_CALLBACK((+[](GtkEditable *editable, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     const char *name = gtk_editable_get_text(editable);
                     GtkWidget *save = GTK_WIDGET(g_object_get_data(G_OBJECT(editable), "save"));
                     const std::string text = name ? name : "";
                     if (save) {
                       gtk_widget_set_sensitive(save, EqualizerPresets::CanSave(text, eq->IsBuiltin(text)));
                     }
                   })),
                   equalizer);
  g_signal_connect(save_preset, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkWidget *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "name"));
                     const char *name = gtk_editable_get_text(GTK_EDITABLE(entry));
                     const std::string text = name ? name : "";
                     if (!EqualizerPresets::CanSave(text, eq->IsBuiltin(text))) {
                       return;
                     }
                     if (eq->SavePreset(text)) {
                       gtk_string_list_append(GTK_STRING_LIST(g_object_get_data(G_OBJECT(button), "list")), text.c_str());
                     }
                   })),
                   equalizer);
  g_object_set_data(G_OBJECT(delete_preset), "drop", preset);
  g_signal_connect(delete_preset, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkDropDown *drop = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "drop"));
                     GtkStringObject *item = drop ? GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop)) : nullptr;
                     if (!item) {
                       return;
                     }
                     const std::string name = gtk_string_object_get_string(item);
                     if (name.empty() || eq->IsBuiltin(name)) {
                       return;
                     }
                     AdwAlertDialog *confirm = ADW_ALERT_DIALOG(
                         adw_alert_dialog_new(Translations::CStr(EqualizerLabels::DeletePreset()),
                                              EqualizerPresets::ConfirmDeleteMessage(name).c_str()));
                     adw_alert_dialog_add_responses(confirm, "no", Translations::CStr("No"), "yes", Translations::CStr("Yes"), nullptr);
                     adw_alert_dialog_set_response_appearance(confirm, "yes", ADW_RESPONSE_DESTRUCTIVE);
                     adw_alert_dialog_set_default_response(confirm, "no");
                     adw_alert_dialog_set_close_response(confirm, "no");
                     auto *job = new EqualizerDeleteJob{eq, drop};
                     g_object_set_data_full(G_OBJECT(confirm), "job", job, [](gpointer p) { delete static_cast<EqualizerDeleteJob *>(p); });
                     g_object_set_data_full(G_OBJECT(confirm), "preset-name", g_strdup(name.c_str()), g_free);
                     g_signal_connect(confirm, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer) {
                                        if (g_strcmp0(response, "yes") != 0) {
                                          return;
                                        }
                                        auto *pending = static_cast<EqualizerDeleteJob *>(g_object_get_data(G_OBJECT(alert), "job"));
                                        const char *preset = static_cast<const char *>(g_object_get_data(G_OBJECT(alert), "preset-name"));
                                        if (pending && preset) {
                                          ApplyEqualizerDelete(pending->eq, pending->drop, preset);
                                        }
                                      }),
                                      nullptr);
                     adw_dialog_present(ADW_DIALOG(confirm), GTK_WIDGET(button));
                   })),
                   equalizer);
  gtk_box_append(GTK_BOX(preset_row), preset_name);
  gtk_box_append(GTK_BOX(preset_row), save_preset);
  gtk_box_append(GTK_BOX(preset_row), delete_preset);
  gtk_box_append(GTK_BOX(box), preset_row);
  GtkWidget *slider_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *preamp_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  GtkWidget *preamp_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *preamp_title = gtk_label_new(Translations::CStr("Preamp"));
  gtk_widget_set_hexpand(preamp_title, TRUE);
  gtk_label_set_xalign(GTK_LABEL(preamp_title), 0);
  GtkWidget *preamp_db = gtk_label_new(EqualizerPersist::DbLabel(equalizer->preamp()).c_str());
  gtk_widget_add_css_class(preamp_db, "dim-label");
  gtk_box_append(GTK_BOX(preamp_header), preamp_title);
  gtk_box_append(GTK_BOX(preamp_header), preamp_db);
  GtkWidget *preamp = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12, 12, 1);
  gtk_range_set_value(GTK_RANGE(preamp), equalizer->preamp());
  gtk_widget_set_tooltip_text(preamp, "Preamp");
  g_object_set_data(G_OBJECT(preamp), "db-label", preamp_db);
  g_signal_connect(preamp, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     const int value = static_cast<int>(gtk_range_get_value(range));
                     static_cast<class Equalizer *>(data)->set_preamp(value);
                     if (auto *label = GTK_LABEL(g_object_get_data(G_OBJECT(range), "db-label"))) {
                       gtk_label_set_text(label, EqualizerPersist::DbLabel(value).c_str());
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(preamp_box), preamp_header);
  gtk_box_append(GTK_BOX(preamp_box), preamp);
  gtk_box_append(GTK_BOX(slider_container), preamp_box);
  GtkWidget *bands = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  static const char *kHz[] = {"60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k"};
  for (int i = 0; i < 10; ++i) {
    GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, EqualizerGain::kSliderMin, EqualizerGain::kSliderMax, 1);
    gtk_range_set_inverted(GTK_RANGE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), equalizer->gains()[static_cast<size_t>(i)]);
    gtk_widget_set_size_request(scale, -1, 160);
    g_object_set_data(G_OBJECT(scale), "band", GINT_TO_POINTER(i));
    GtkWidget *db = gtk_label_new(EqualizerGain::BandDbLabel(equalizer->gains()[static_cast<size_t>(i)]).c_str());
    gtk_widget_add_css_class(db, "dim-label");
    g_object_set_data(G_OBJECT(scale), "db-label", db);
    g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                       const int band = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(range), "band"));
                       const int value = EqualizerGain::ClampSlider(static_cast<int>(gtk_range_get_value(range)));
                       static_cast<class Equalizer *>(data)->set_gain(band, value);
                       if (auto *label = GTK_LABEL(g_object_get_data(G_OBJECT(range), "db-label"))) {
                         gtk_label_set_text(label, EqualizerGain::BandDbLabel(value).c_str());
                       }
                     }),
                     equalizer);
    gtk_box_append(GTK_BOX(col), scale);
    gtk_box_append(GTK_BOX(col), db);
    gtk_box_append(GTK_BOX(col), gtk_label_new(kHz[i]));
    gtk_box_append(GTK_BOX(bands), col);
  }
  gtk_box_append(GTK_BOX(slider_container), bands);
  gtk_widget_set_sensitive(slider_container, EqualizerControls::SliderContainerEnabled(equalizer->enabled()));
  g_object_set_data(G_OBJECT(enable), "slider-container", slider_container);
  g_signal_connect(enable, "toggled", G_CALLBACK((+[](GtkCheckButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     eq->set_enabled(gtk_check_button_get_active(button));
                     if (auto *container = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "slider-container"))) {
                       gtk_widget_set_sensitive(container, EqualizerControls::SliderContainerEnabled(eq->enabled()));
                     }
                   })),
                   equalizer);
  gtk_box_append(GTK_BOX(box), slider_container);
  GtkWidget *enable_balance = gtk_check_button_new_with_label(Translations::CStr(EqualizerLabels::EnableBalancer()));
  gtk_widget_set_tooltip_text(enable_balance, Translations::CStr(EqualizerLabels::RestartHint()));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(enable_balance), equalizer->stereo_balancer_enabled());
  GtkWidget *balance_hint = gtk_label_new(Translations::CStr(EqualizerLabels::RestartHint()));
  gtk_label_set_wrap(GTK_LABEL(balance_hint), TRUE);
  gtk_widget_set_halign(balance_hint, GTK_ALIGN_START);
  gtk_widget_add_css_class(balance_hint, "dim-label");
  GtkWidget *balance_ends = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *left_label = gtk_label_new(Translations::CStr(EqualizerLabels::Left()));
  GtkWidget *balance_label = gtk_label_new(Translations::CStr(EqualizerLabels::Balance()));
  GtkWidget *right_label = gtk_label_new(Translations::CStr(EqualizerLabels::Right()));
  gtk_widget_set_hexpand(balance_label, TRUE);
  gtk_widget_set_halign(balance_label, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(balance_ends), left_label);
  gtk_box_append(GTK_BOX(balance_ends), balance_label);
  gtk_box_append(GTK_BOX(balance_ends), right_label);
  GtkWidget *balance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -100, 100, 1);
  gtk_range_set_value(GTK_RANGE(balance_scale), equalizer->stereo_balance());
  gtk_scale_add_mark(GTK_SCALE(balance_scale), 0, GTK_POS_BOTTOM, Translations::CStr(EqualizerLabels::Balance()));
  gtk_widget_set_tooltip_text(balance_scale, Translations::CStr(EqualizerLabels::Balance()));
  gtk_widget_set_sensitive(balance_scale, EqualizerControls::StereoBalanceSliderEnabled(equalizer->stereo_balancer_enabled()));
  gtk_widget_set_sensitive(balance_ends, EqualizerControls::StereoBalanceLabelsEnabled(equalizer->stereo_balancer_enabled()));
  g_object_set_data(G_OBJECT(enable_balance), "scale", balance_scale);
  g_object_set_data(G_OBJECT(enable_balance), "labels", balance_ends);
  g_object_set_data(G_OBJECT(enable_balance), "equalizer-app", app);
  g_signal_connect(enable_balance, "toggled", G_CALLBACK((+[](GtkCheckButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     const bool on = gtk_check_button_get_active(button);
                     GtkWidget *scale = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "scale"));
                     GtkWidget *labels = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "labels"));
                     if (scale) {
                       gtk_range_set_value(GTK_RANGE(scale), EqualizerControls::DisplayBalanceAfterToggle(on, eq->stereo_balance()));
                     }
                     eq->set_stereo_balancer_enabled(on);
                     if (scale) {
                       gtk_widget_set_sensitive(scale, EqualizerControls::StereoBalanceSliderEnabled(on));
                     }
                     if (labels) {
                       gtk_widget_set_sensitive(labels, EqualizerControls::StereoBalanceLabelsEnabled(on));
                     }
                     if (auto *application = static_cast<Application *>(g_object_get_data(G_OBJECT(button), "equalizer-app"))) {
                       application->player()->engine()->SetStereoBalance(eq->EffectiveBalanceFraction());
                     }
                   })),
                   equalizer);
  g_object_set_data(G_OBJECT(balance_scale), "equalizer-app", app);
  g_signal_connect(balance_scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     eq->set_stereo_balance(static_cast<int>(gtk_range_get_value(range)));
                     if (auto *application = static_cast<Application *>(g_object_get_data(G_OBJECT(range), "equalizer-app"))) {
                       application->player()->engine()->SetStereoBalance(eq->EffectiveBalanceFraction());
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), enable_balance);
  gtk_box_append(GTK_BOX(box), balance_hint);
  gtk_box_append(GTK_BOX(box), balance_ends);
  gtk_box_append(GTK_BOX(box), balance_scale);
  DialogChrome::SetContent(dialog, box);
  DialogCloseKeys::Attach(dialog);
  g_signal_connect(dialog, "closed",
                   G_CALLBACK((+[](AdwDialog *, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     if (eq) {
                       PromptSaveIfDirty(nullptr, eq, eq->selected_preset(), {});
                     }
                   })),
                   equalizer);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
