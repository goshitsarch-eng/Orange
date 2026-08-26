#include "widgets/prettyslider.h"

PrettySlider::PrettySlider(double min, double max, double step) : SliderSlider(min, max, step) {
  gtk_widget_add_css_class(widget(), "pretty-slider");
}

void PrettySlider::SetPopupText(const std::string &text) {
  popup_text_ = text;
  gtk_widget_set_tooltip_text(widget(), text.c_str());
}
