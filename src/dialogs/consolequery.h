#ifndef STRAWBERRY_CONSOLEQUERY_H
#define STRAWBERRY_CONSOLEQUERY_H

#include "core/database.h"

#include <string>
#include <vector>

namespace ConsoleQuery {

struct Result {
  bool ok = false;
  std::string sql;
  std::string error;
  std::vector<std::string> columns;
  std::vector<std::vector<std::string>> rows;
  int changes = 0;
};

inline std::string JoinRow(const std::vector<std::string> &values, char separator = '|') {
  std::string out;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out.push_back(separator);
    }
    out += values[i];
  }
  return out;
}

inline std::string Format(const Result &result) {
  std::string out = "> " + result.sql + "\n";
  if (!result.ok) {
    if (!result.error.empty()) {
      out += result.error + "\n";
    }
    return out;
  }
  for (const auto &row : result.rows) {
    out += JoinRow(row) + "\n";
  }
  return out;
}

inline Result Run(Database *db, const std::string &sql) {
  Result result;
  result.sql = sql;
  if (!db || !db->handle()) {
    result.error = "no database";
    return result;
  }
  sqlite3_stmt *stmt = nullptr;
  if (!db->Prepare(sql, &stmt) || !stmt) {
    result.error = db->LastError();
    if (stmt) {
      sqlite3_finalize(stmt);
    }
    return result;
  }
  const int columns = sqlite3_column_count(stmt);
  result.columns.reserve(static_cast<size_t>(columns));
  for (int i = 0; i < columns; ++i) {
    const char *name = sqlite3_column_name(stmt, i);
    result.columns.emplace_back(name ? name : "");
  }
  while (true) {
    const int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      std::vector<std::string> row;
      row.reserve(static_cast<size_t>(columns));
      for (int i = 0; i < columns; ++i) {
        const unsigned char *text = sqlite3_column_text(stmt, i);
        row.emplace_back(text ? reinterpret_cast<const char *>(text) : "");
      }
      result.rows.push_back(std::move(row));
    } else if (rc == SQLITE_DONE) {
      result.ok = true;
      result.changes = sqlite3_changes(db->handle());
      break;
    } else {
      result.error = db->LastError();
      break;
    }
  }
  sqlite3_finalize(stmt);
  return result;
}

}  // namespace ConsoleQuery

#endif
