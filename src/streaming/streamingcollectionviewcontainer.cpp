#include "streaming/streamingcollectionviewcontainer.h"

#include "collection/groupbydialog.h"
#include "streaming/streamingabort.h"
#include "translations/translations.h"

StreamingCollectionViewContainer::StreamingCollectionViewContainer(const std::string &title)
    : filter_widget_(std::make_unique<CollectionFilterWidget>()), view_(std::make_unique<StreamingCollectionView>(title)) {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  progress_ = gtk_progress_bar_new();
  gtk_widget_set_margin_start(progress_, 8);
  gtk_widget_set_margin_end(progress_, 8);
  gtk_widget_set_visible(progress_, FALSE);
  status_ = gtk_label_new("");
  gtk_widget_set_halign(status_, GTK_ALIGN_START);
  gtk_widget_set_margin_start(status_, 8);
  gtk_widget_set_margin_end(status_, 8);
  gtk_widget_set_visible(status_, FALSE);
  abort_ = gtk_button_new_with_label(Translations::CStr(StreamingAbort::AbortLabel()));
  gtk_widget_set_halign(abort_, GTK_ALIGN_END);
  gtk_widget_set_margin_start(abort_, 8);
  gtk_widget_set_margin_end(abort_, 8);
  gtk_widget_set_margin_bottom(abort_, 8);
  gtk_widget_set_visible(abort_, FALSE);
  g_signal_connect(abort_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     static_cast<StreamingCollectionViewContainer *>(data)->OnActionClicked();
                   }),
                   this);
  filter_widget_->SetChangedCallback([this]() { view_->SetFilterOptions(filter_widget_->options()); });
  filter_widget_->SetGroupingChangedCallback([this](const CollectionGrouping::Grouping &grouping) { view_->ApplyGrouping(grouping); });
  filter_widget_->SetMenuActionCallback([this](CollectionFilterMenu::ActionKind kind) {
    if (kind == CollectionFilterMenu::ActionKind::Advanced) {
      GtkRoot *root = gtk_widget_get_root(widget_);
      GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;
      GroupByDialog::Show(parent, view_->grouping(), [this](const CollectionGrouping::Grouping &grouping) {
        view_->ApplyGrouping(grouping);
      });
      return;
    }
    if (menu_action_) {
      menu_action_(kind);
    }
  });
  gtk_box_append(GTK_BOX(widget_), filter_widget_->widget());
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  gtk_box_append(GTK_BOX(widget_), progress_);
  gtk_box_append(GTK_BOX(widget_), status_);
  gtk_box_append(GTK_BOX(widget_), abort_);
}

void StreamingCollectionViewContainer::UpdateActionButton() {
  if (!abort_) {
    return;
  }
  gtk_button_set_label(GTK_BUTTON(abort_), Translations::CStr(StreamingAbort::ButtonLabel(working_, has_error_)));
  gtk_widget_set_visible(abort_, StreamingAbort::ShouldShowAction(working_, has_error_) ? TRUE : FALSE);
}

void StreamingCollectionViewContainer::OnActionClicked() {
  if (working_) {
    if (abort_callback_) {
      abort_callback_();
    }
    return;
  }
  HideProgress();
}

void StreamingCollectionViewContainer::ShowProgress(const std::string &status) {
  working_ = true;
  has_error_ = false;
  if (!status.empty()) {
    SetProgressStatus(status);
  }
  if (progress_) {
    if (!gtk_widget_get_visible(progress_)) {
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    }
    gtk_widget_set_visible(progress_, TRUE);
  }
  UpdateActionButton();
}

void StreamingCollectionViewContainer::ShowError(const std::string &status) {
  working_ = false;
  has_error_ = true;
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    gtk_widget_set_visible(progress_, FALSE);
  }
  SetProgressStatus(status);
  UpdateActionButton();
}

void StreamingCollectionViewContainer::HideProgress() {
  working_ = false;
  has_error_ = false;
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), 0);
    gtk_widget_set_visible(progress_, FALSE);
  }
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), "");
    gtk_widget_set_visible(status_, FALSE);
  }
  UpdateActionButton();
}

void StreamingCollectionViewContainer::HideProgressUnlessError() {
  if (!has_error_) {
    HideProgress();
  }
}

void StreamingCollectionViewContainer::SetProgress(int value, int maximum) {
  working_ = true;
  has_error_ = false;
  if (maximum > 0) {
    progress_max_ = maximum;
  }
  if (progress_) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), StreamingProgress::Fraction(value, progress_max_));
    gtk_widget_set_visible(progress_, TRUE);
  }
  UpdateActionButton();
}

void StreamingCollectionViewContainer::SetAbortCallback(AbortCallback callback) { abort_callback_ = std::move(callback); }

void StreamingCollectionViewContainer::SetProgressMaximum(int maximum) {
  if (maximum > 0) {
    progress_max_ = maximum;
  }
}

void StreamingCollectionViewContainer::SetProgressStatus(const std::string &status) {
  if (status_) {
    gtk_label_set_text(GTK_LABEL(status_), status.c_str());
    gtk_widget_set_visible(status_, !status.empty());
  }
}
