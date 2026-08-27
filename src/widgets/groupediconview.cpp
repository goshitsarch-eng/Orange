#include "groupediconview.h"

#include "dialogs/dialoghelpers.h"

#include <map>

GroupedIconView::GroupedIconView() {
  root_ = gtk_scrolled_window_new();
  g_object_ref_sink(root_);
  gtk_widget_set_vexpand(root_, TRUE);
  box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box_, 8);
  gtk_widget_set_margin_end(box_, 8);
  gtk_widget_set_margin_top(box_, 8);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(root_), box_);
}

GroupedIconView::~GroupedIconView() {
  if (root_) g_object_unref(root_);
}

void GroupedIconView::Clear() {
  GtkWidget *child = gtk_widget_get_first_child(box_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(box_), child);
    child = next;
  }
  item_count_ = 0;
}

void GroupedIconView::SetItems(const std::vector<Item> &items) {
  Clear();
  std::map<std::string, std::vector<int>> groups;
  for (int i = 0; i < static_cast<int>(items.size()); ++i) {
    groups[items[static_cast<size_t>(i)].group].push_back(i);
  }
  for (const auto &group : groups) {
    if (!group.first.empty()) {
      GtkWidget *header = gtk_label_new(group.first.c_str());
      gtk_widget_set_halign(header, GTK_ALIGN_START);
      gtk_widget_add_css_class(header, "heading");
      gtk_box_append(GTK_BOX(box_), header);
    }
    GtkWidget *flow = gtk_flow_box_new();
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 6);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_SINGLE);
    for (int index : group.second) {
      const Item &item = items[static_cast<size_t>(index)];
      GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
      gtk_widget_set_size_request(card, 120, -1);
      GtkWidget *image = gtk_image_new();
      DialogHelpers::SetImageFromBytes(image, item.image, 96);
      GtkWidget *title = gtk_label_new(item.title.c_str());
      gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
      GtkWidget *subtitle = gtk_label_new(item.subtitle.c_str());
      gtk_widget_add_css_class(subtitle, "dim-label");
      gtk_label_set_ellipsize(GTK_LABEL(subtitle), PANGO_ELLIPSIZE_END);
      gtk_box_append(GTK_BOX(card), image);
      gtk_box_append(GTK_BOX(card), title);
      gtk_box_append(GTK_BOX(card), subtitle);
      gtk_flow_box_append(GTK_FLOW_BOX(flow), card);
      g_object_set_data(G_OBJECT(card), "item-index", GINT_TO_POINTER(index));
    }
    g_signal_connect(flow, "child-activated", G_CALLBACK(+[](GtkFlowBox *, GtkFlowBoxChild *child, gpointer self) {
                       auto *view = static_cast<GroupedIconView *>(self);
                       GtkWidget *card = gtk_flow_box_child_get_child(child);
                       const int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(card), "item-index"));
                       if (view->activate_cb_) view->activate_cb_(index);
                     }),
                     this);
    gtk_box_append(GTK_BOX(box_), flow);
  }
  item_count_ = static_cast<int>(items.size());
}
