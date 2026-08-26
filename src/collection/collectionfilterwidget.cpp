#include "collection/collectionfilterwidget.h"

CollectionFilterWidget::CollectionFilterWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(widget_, 8);
  gtk_widget_set_margin_end(widget_, 8);
  gtk_widget_set_margin_top(widget_, 6);
  gtk_widget_set_margin_bottom(widget_, 4);
  static const char *age_labels[] = {"Any age", "Added today", "Added last week", "Added last month", "Added last 3 months",
                                     "Added last year", nullptr};
  static const char *rating_labels[] = {"Any rating", "Unrated", "1★+", "2★+", "3★+", "4★+", "5★", nullptr};
  static const char *mode_labels[] = {"All songs", "Duplicates", "Untagged", nullptr};
  age_drop_ = gtk_drop_down_new_from_strings(age_labels);
  rating_drop_ = gtk_drop_down_new_from_strings(rating_labels);
  mode_drop_ = gtk_drop_down_new_from_strings(mode_labels);
  gtk_widget_set_hexpand(age_drop_, TRUE);
  gtk_widget_set_hexpand(rating_drop_, TRUE);
  gtk_widget_set_hexpand(mode_drop_, TRUE);
  gtk_box_append(GTK_BOX(widget_), age_drop_);
  gtk_box_append(GTK_BOX(widget_), rating_drop_);
  gtk_box_append(GTK_BOX(widget_), mode_drop_);
  auto notify = +[](GtkDropDown *, GParamSpec *, gpointer data) {
    static_cast<CollectionFilterWidget *>(data)->EmitChanged();
  };
  g_signal_connect(age_drop_, "notify::selected", G_CALLBACK(notify), this);
  g_signal_connect(rating_drop_, "notify::selected", G_CALLBACK(notify), this);
  g_signal_connect(mode_drop_, "notify::selected", G_CALLBACK(notify), this);
}

void CollectionFilterWidget::SetChangedCallback(ChangedCallback callback) { changed_ = std::move(callback); }

void CollectionFilterWidget::EmitChanged() {
  static const int days[] = {-1, 1, 7, 30, 90, 365};
  static const float ratings[] = {-1.0f, -2.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
  const guint age = gtk_drop_down_get_selected(GTK_DROP_DOWN(age_drop_));
  const guint rating = gtk_drop_down_get_selected(GTK_DROP_DOWN(rating_drop_));
  const guint mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(mode_drop_));
  options_.set_max_age(age < G_N_ELEMENTS(days) && days[age] > 0 ? days[age] * 86400 : -1);
  unrated_only_ = rating < G_N_ELEMENTS(ratings) && ratings[rating] <= -2.0f;
  options_.set_min_rating(unrated_only_ ? -1.0f : (rating < G_N_ELEMENTS(ratings) ? ratings[rating] : -1.0f));
  if (mode == 1) {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  } else if (mode == 2) {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  } else {
    options_.set_filter_mode(CollectionFilterOptions::FilterMode::All);
  }
  if (changed_) {
    changed_();
  }
}
