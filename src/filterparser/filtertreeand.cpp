#include "filterparser/filtertreeand.h"

void FilterTreeAnd::Add(std::unique_ptr<FilterTree> child) {
  if (child) {
    children_.push_back(std::move(child));
  }
}

std::string FilterTreeAnd::ToSql() const {
  if (children_.empty()) {
    return "1=1";
  }
  std::string sql = "(";
  for (size_t i = 0; i < children_.size(); ++i) {
    if (i) {
      sql += " AND ";
    }
    sql += children_[i] ? children_[i]->ToSql() : "1=1";
  }
  sql += ")";
  return sql;
}

bool FilterTreeAnd::accept(const Song &song) const {
  for (const auto &child : children_) {
    if (child && !child->accept(song)) {
      return false;
    }
  }
  return true;
}
