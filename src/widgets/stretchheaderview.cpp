#include "widgets/stretchheaderview.h"

#include <algorithm>

void StretchHeaderView::SetColumns(const std::vector<std::string> &names) { columns_ = names; }

void StretchHeaderView::SetStretchColumn(int column) {
  if (column >= 0 && column < static_cast<int>(columns_.size())) {
    stretch_column_ = column;
  }
}

double StretchHeaderView::ColumnWidth(int column, double total_width) const {
  if (columns_.empty() || total_width <= 0) {
    return 0;
  }
  const double fixed = 96.0;
  const int stretch = stretch_column_ >= 0 && stretch_column_ < static_cast<int>(columns_.size()) ? stretch_column_ : 0;
  if (column == stretch) {
    const double others = fixed * static_cast<double>(columns_.size() - 1);
    return std::max(fixed, total_width - others);
  }
  return fixed;
}
