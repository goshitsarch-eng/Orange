#ifndef STRAWBERRY_DATABASE_H
#define STRAWBERRY_DATABASE_H

#include "core/signal.h"
#include "core/song.h"

#include <sqlite3.h>

#include <functional>
#include <mutex>
#include <string>
#include <vector>

class Database {
 public:
  explicit Database(const std::string &path = {});
  ~Database();

  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

  bool Open();
  void Close();
  sqlite3 *handle() const { return db_; }
  const std::string &path() const { return path_; }

  bool Exec(const std::string &sql);
  bool Prepare(const std::string &sql, sqlite3_stmt **stmt) const;

  int64_t LastInsertRowId() const;
  std::string LastError() const;

  void Backup();

  Signal<> BackupFinished;

  static constexpr int kCurrentSchemaVersion = 23;
  int startup_schema_version() const { return startup_schema_version_; }
  int current_schema_version() const { return kCurrentSchemaVersion; }

  static std::string SchemaSql();

 private:
  bool Migrate();
  bool ApplySchemaFile(int version);

  std::string path_;
  sqlite3 *db_ = nullptr;
  mutable std::mutex mutex_;
  int startup_schema_version_ = 0;
};

class SqlQuery {
 public:
  SqlQuery(Database *db, const std::string &sql);
  ~SqlQuery();

  SqlQuery(const SqlQuery &) = delete;
  SqlQuery &operator=(const SqlQuery &) = delete;

  void Bind(int index, const std::string &value);
  void Bind(int index, int value);
  void Bind(int index, int64_t value);
  void Bind(int index, double value);
  void BindNull(int index);

  bool Step();
  bool Exec();

  int ColumnInt(int index) const;
  int64_t ColumnInt64(int index) const;
  double ColumnDouble(int index) const;
  std::string ColumnText(int index) const;
  bool ColumnIsNull(int index) const;

  sqlite3_stmt *stmt() const { return stmt_; }

 private:
  sqlite3_stmt *stmt_ = nullptr;
};

#endif
