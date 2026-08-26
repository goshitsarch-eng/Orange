#ifndef STRAWBERRY_FILTERTREE_H
#define STRAWBERRY_FILTERTREE_H

#include "core/song.h"
#include "filterparser/filtercolumn.h"

#include <memory>
#include <string>
#include <vector>

class FilterTree {
 public:
  enum class FilterType { Nop = 0, Or, And, Not, Column, Term };

  virtual ~FilterTree() = default;
  virtual FilterType type() const = 0;
  virtual bool accept(const Song &song) const = 0;
  virtual std::string ToSql() const { return "1=1"; }

  static std::string DataFromColumn(FilterColumn column, const Song &song);
  static double NumericFromColumn(FilterColumn column, const Song &song);
  static bool IsNumeric(FilterColumn column);
};

#endif
