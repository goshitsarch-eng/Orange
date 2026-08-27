#include "collection/collectionviewcontainer.h"

#include "translations/translations.h"

CollectionViewContainer::CollectionViewContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),
      filter_widget_(std::make_unique<CollectionFilterWidget>()),
      view_(std::make_unique<CollectionView>()) {
  GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(bar, 8);
  gtk_widget_set_margin_end(bar, 8);
  GtkWidget *expand = gtk_button_new_from_icon_name("pan-down-symbolic");
  gtk_widget_add_css_class(expand, "flat");
  gtk_widget_set_tooltip_text(expand, Translations::CStr("Expand all"));
  GtkWidget *collapse = gtk_button_new_from_icon_name("pan-end-symbolic");
  gtk_widget_add_css_class(collapse, "flat");
  gtk_widget_set_tooltip_text(collapse, Translations::CStr("Collapse all"));
  g_signal_connect(expand, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<CollectionView *>(data)->ExpandAll();
                   }),
                   view_.get());
  g_signal_connect(collapse, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<CollectionView *>(data)->CollapseAll();
                   }),
                   view_.get());
  gtk_box_append(GTK_BOX(bar), expand);
  gtk_box_append(GTK_BOX(bar), collapse);
  gtk_box_append(GTK_BOX(widget_), filter_widget_->widget());
  gtk_box_append(GTK_BOX(widget_), bar);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
}
