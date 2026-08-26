#ifndef STRAWBERRY_EQUALIZERSLIDER_H
#define STRAWBERRY_EQUALIZERSLIDER_H

#include <gtk/gtk.h>

#include <string>

class EqualizerSlider {
 public:
  EqualizerSlider(const std::string &label, int band, int value);
  GtkWidget *widget() const { return widget_; }
  int band() const { return band_; }
  int value() const { return value_; }
  void set_value(int value);

  using ChangedCallback = void (*)(int band, int value, gpointer data);
  void set_changed_callback(ChangedCallback callback, gpointer data);

 private:
  static void OnValueChanged(GtkRange *range, gpointer data);

  GtkWidget *widget_ = nullptr;
  GtkWidget *scale_ = nullptr;
  int band_ = 0;
  int value_ = 0;
  ChangedCallback callback_ = nullptr;
  gpointer callback_data_ = nullptr;
};

#endif
