#ifndef STRAWBERRY_COLLECTIONQUERY_H
#define STRAWBERRY_COLLECTIONQUERY_H

#include "collection/collectionfilteroptions.h"
#include "core/database.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CollectionQuery {
 public:
  CollectionQuery(Database *db, const std::string &songs_table, const CollectionFilterOptions &filter_options = CollectionFilterOptions());

  void SetColumnSpec(const std::string &column_spec) { column_spec_ = column_spec; }
  void SetOrderBy(const std::string &order_by) { order_by_ = order_by; }
  void AddWhere(const std::string &column, const std::string &value, const std::string &op = "=");
  void AddWhere(const std::string &column, int value, const std::string &op = "=");
  void AddWhere(const std::string &column, int64_t value, const std::string &op = "=");
  void AddWhereIn(const std::string &column, const std::vector<std::string> &values);
  void AddWhereClause(const std::string &clause);
  void AddCompilationRequirement(bool compilation);
  void SetDuplicatesOnly(bool duplicates_only) { duplicates_only_ = duplicates_only; }
  void SetIncludeUnavailable(bool include_unavailable) { include_unavailable_ = include_unavailable; }
  void SetLimit(int limit) { limit_ = limit; }

  const std::string &column_spec() const { return column_spec_; }
  const std::string &order_by() const { return order_by_; }
  const std::vector<std::string> &where_clauses() const { return where_clauses_; }
  bool include_unavailable() const { return include_unavailable_; }
  bool duplicates_only() const { return duplicates_only_; }
  int limit() const { return limit_; }

  std::string Sql() const;
  std::string GetInnerQuery() const;

  bool Exec();
  bool Next();
  SqlQuery *query() const { return query_.get(); }

 private:
  struct BoundValue {
    enum class Type { Text, Int, Int64 };
    Type type = Type::Text;
    std::string text;
    int64_t number = 0;
  };

  Database *db_ = nullptr;
  std::string songs_table_;
  std::string column_spec_ = "*";
  std::string order_by_;
  std::vector<std::string> where_clauses_;
  std::vector<BoundValue> bound_values_;
  bool include_unavailable_ = false;
  bool duplicates_only_ = false;
  int limit_ = -1;
  std::unique_ptr<SqlQuery> query_;
};

#endif
