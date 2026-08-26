#include "collection/collectionquery.h"

#include "utilities/strutils.h"

#include <ctime>

CollectionQuery::CollectionQuery(Database *db, const std::string &songs_table, const CollectionFilterOptions &filter_options)
    : db_(db), songs_table_(songs_table) {
  if (filter_options.max_age() != -1) {
    const int64_t cutoff = static_cast<int64_t>(std::time(nullptr)) - filter_options.max_age();
    AddWhere("ctime", cutoff, ">");
  }
  duplicates_only_ = filter_options.filter_mode() == CollectionFilterOptions::FilterMode::Duplicates;
  if (filter_options.filter_mode() == CollectionFilterOptions::FilterMode::Untagged) {
    AddWhereClause("(artist = '' OR album = '' OR title = '')");
  }
}

void CollectionQuery::AddWhere(const std::string &column, const std::string &value, const std::string &op) {
  where_clauses_.push_back(column + " " + op + " ?");
  BoundValue bound;
  bound.type = BoundValue::Type::Text;
  bound.text = value;
  bound_values_.push_back(bound);
}

void CollectionQuery::AddWhere(const std::string &column, int value, const std::string &op) {
  where_clauses_.push_back(column + " " + op + " " + std::to_string(value));
}

void CollectionQuery::AddWhere(const std::string &column, int64_t value, const std::string &op) {
  where_clauses_.push_back(column + " " + op + " " + std::to_string(value));
}

void CollectionQuery::AddWhereIn(const std::string &column, const std::vector<std::string> &values) {
  if (values.empty()) {
    where_clauses_.push_back("0");
    return;
  }
  std::string clause = column + " IN (";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      clause += ",";
    }
    clause += "?";
    BoundValue bound;
    bound.type = BoundValue::Type::Text;
    bound.text = values[i];
    bound_values_.push_back(bound);
  }
  clause += ")";
  where_clauses_.push_back(clause);
}

void CollectionQuery::AddWhereClause(const std::string &clause) {
  if (!clause.empty()) {
    where_clauses_.push_back(clause);
  }
}

void CollectionQuery::AddCompilationRequirement(bool compilation) {
  where_clauses_.push_back("+compilation_effective = " + std::to_string(compilation ? 1 : 0));
}

std::string CollectionQuery::GetInnerQuery() const {
  if (!duplicates_only_) {
    return {};
  }
  return " INNER JOIN (select * from duplicated_songs) dsongs ON (" + songs_table_ + ".artist = dsongs.dup_artist AND " +
         songs_table_ + ".album = dsongs.dup_album AND " + songs_table_ + ".title = dsongs.dup_title)";
}

std::string CollectionQuery::Sql() const {
  std::string sql = "SELECT " + column_spec_ + " FROM " + songs_table_ + GetInnerQuery();
  std::vector<std::string> where = where_clauses_;
  if (!include_unavailable_) {
    where.push_back("unavailable = 0");
  }
  if (!where.empty()) {
    sql += " WHERE " + StrUtils::Join(where, " AND ");
  }
  if (!order_by_.empty()) {
    sql += " ORDER BY " + order_by_;
  }
  if (limit_ != -1) {
    sql += " LIMIT " + std::to_string(limit_);
  }
  return sql;
}

bool CollectionQuery::Exec() {
  if (!db_) {
    return false;
  }
  query_ = std::make_unique<SqlQuery>(db_, Sql());
  int index = 1;
  for (const BoundValue &value : bound_values_) {
    if (value.type == BoundValue::Type::Text) {
      query_->Bind(index, value.text);
    } else if (value.type == BoundValue::Type::Int64) {
      query_->Bind(index, value.number);
    } else {
      query_->Bind(index, static_cast<int>(value.number));
    }
    ++index;
  }
  return true;
}

bool CollectionQuery::Next() { return query_ && query_->Step(); }
