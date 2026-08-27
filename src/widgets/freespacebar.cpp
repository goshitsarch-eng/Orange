#include "freespacebar.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "utilities/fileutils.h"

FreeSpaceBar::FreeSpaceBar() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  bar_ = gtk_progress_bar_new();
  label_ = gtk_label_new("");
  gtk_widget_set_halign(label_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(root_), bar_);
  gtk_box_append(GTK_BOX(root_), label_);
}

FreeSpaceBar::~FreeSpaceBar() { root_ = nullptr; }

void FreeSpaceBar::SetBytes(int64_t used, int64_t additional, int64_t total) {
  used_ = used;
  additional_ = additional;
  total_ = total;
  Refresh();
}

void FreeSpaceBar::SetPath(const std::string &path) {
  const int64_t free = FileUtils::FreeSpaceBytes(path);
  const int64_t total = FileUtils::TotalSpaceBytes(path);
  if (free < 0 || total <= 0) {
    SetBytes(0, 0, 0);
    return;
  }
  SetBytes(std::max<int64_t>(0, total - free), 0, total);
}

double FreeSpaceBar::fraction() const {
  if (total_ <= 0) return 0.0;
  return std::min(1.0, static_cast<double>(used_ + additional_) / static_cast<double>(total_));
}

void FreeSpaceBar::Refresh() {
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar_), fraction());
  std::ostringstream out;
  if (total_ <= 0) {
    out << "Free space unknown";
  } else {
    const int64_t remaining = std::max<int64_t>(0, total_ - used_ - additional_);
    out << FileUtils::PrettySize(remaining) << " free";
    if (additional_ > 0) out << " after copy";
  }
  gtk_label_set_text(GTK_LABEL(label_), out.str().c_str());
}
