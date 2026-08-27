#ifndef STRAWBERRY_STRETCHHEADERVIEW_H
#define STRAWBERRY_STRETCHHEADERVIEW_H

#include <string>
#include <vector>

class StretchHeaderView {
 public:
  void SetColumns(const std::vector<std::string> &names);
  const std::vector<std::string> &columns() const { return columns_; }
  void SetStretchColumn(int column);
  int stretch_column() const { return stretch_column_; }
  double ColumnWidth(int column, double total_width) const;

 private:
  std::vector<std::string> columns_;
  int stretch_column_ = 0;
};

#endif
