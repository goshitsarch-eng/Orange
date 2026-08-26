#ifndef STRAWBERRY_SQLHELPER_H
#define STRAWBERRY_SQLHELPER_H

#include "core/sqlquery.h"
#include "core/sqlrow.h"

#include <string>
#include <vector>

namespace SqlHelper {

inline std::vector<SqlRow> FetchAll(SqlQuery *query) {
  std::vector<SqlRow> rows;
  if (!query) {
    return rows;
  }
  while (query->Step()) {
    SqlRow row;
    for (int i = 0; i < sqlite3_column_count(query->stmt()); ++i) {
      row.Add(query->ColumnIsNull(i) ? std::string() : query->ColumnText(i));
    }
    rows.push_back(row);
  }
  return rows;
}

}  // namespace SqlHelper

#endif
