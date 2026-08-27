#ifndef STRAWBERRY_SQLROW_H
#define STRAWBERRY_SQLROW_H

#include <string>
#include <vector>

class SqlRow {
 public:
  void Add(const std::string &value) { values_.push_back(value); }
  const std::string &value(int index) const { return values_.at(static_cast<size_t>(index)); }
  int columns() const { return static_cast<int>(values_.size()); }
  const std::vector<std::string> &values() const { return values_; }

 private:
  std::vector<std::string> values_;
};

#endif
