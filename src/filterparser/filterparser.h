#ifndef STRAWBERRY_FILTERPARSER_H
#define STRAWBERRY_FILTERPARSER_H

#include "core/song.h"
#include "filterparser/filtercolumn.h"
#include "filterparser/filtertree.h"

#include <memory>
#include <string>

class FilterParser {
 public:
  explicit FilterParser(const std::string &filter);

  FilterTree *parse() const { return tree_.get(); }
  bool Matches(const Song &song) const;
  const std::string &filter() const { return filter_; }
  std::string ToSql() const;
  static std::string ToolTip();

 private:
  void Parse();
  std::unique_ptr<FilterTree> ParseOr();
  std::unique_ptr<FilterTree> ParseAnd();
  std::unique_ptr<FilterTree> ParseUnary();
  std::unique_ptr<FilterTree> ParseTerm();
  std::unique_ptr<FilterTree> CreateSearchTerm(const std::string &column, const std::string &prefix, const std::string &value) const;
  void SkipSpace();
  bool Consume(const std::string &word);
  static FilterOperator OperatorFromPrefix(const std::string &prefix);
  static FilterColumn ColumnFromName(const std::string &name);
  static bool IsNumeric(FilterColumn column);
  static bool IsTimeDays(FilterColumn column);
  static std::string ColumnSql(FilterColumn column);
  static std::string TermSql(FilterColumn column, FilterOperator op, const std::string &value);
  static std::string FreeTextSql(const std::string &value);

  std::string filter_;
  size_t pos_ = 0;
  std::unique_ptr<FilterTree> tree_;
};

#endif
