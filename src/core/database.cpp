#include "core/database.h"

#include "core/logging.h"
#include "core/standardpaths.h"

#include <gio/gio.h>

#include <cstring>

namespace {

const int kSchemaVersion = Database::kCurrentSchemaVersion;

std::string LoadSchemaResource() {
  GBytes *bytes = g_resources_lookup_data("/org/strawberrymusicplayer/Strawberry/schema/schema.sql", G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
  if (!bytes) {
    return {};
  }
  gsize size = 0;
  const gchar *data = static_cast<const gchar *>(g_bytes_get_data(bytes, &size));
  std::string sql(data, size);
  g_bytes_unref(bytes);
  return sql;
}

}  // namespace

Database::Database(const std::string &path) : path_(path.empty() ? StandardPaths::DatabasePath() : path) {}

Database::~Database() { Close(); }

bool Database::Open() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (db_) {
    return true;
  }
  if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
    LogError("Failed to open database %s: %s", path_.c_str(), LastError().c_str());
    return false;
  }
  sqlite3_busy_timeout(db_, 5000);
  Exec("PRAGMA foreign_keys = ON");
  Exec("PRAGMA journal_mode = WAL");
  return Migrate();
}

void Database::Close() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

bool Database::Exec(const std::string &sql) {
  char *error = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
    LogError("SQL error: %s (%s)", error ? error : "unknown", sql.c_str());
    sqlite3_free(error);
    return false;
  }
  return true;
}

bool Database::Prepare(const std::string &sql, sqlite3_stmt **stmt) const {
  return sqlite3_prepare_v2(db_, sql.c_str(), -1, stmt, nullptr) == SQLITE_OK;
}

int64_t Database::LastInsertRowId() const { return sqlite3_last_insert_rowid(db_); }

std::string Database::LastError() const { return db_ ? sqlite3_errmsg(db_) : "no database"; }

void Database::Backup() {
  const std::string backup_path = path_ + ".bak";
  sqlite3 *backup_db = nullptr;
  if (sqlite3_open(backup_path.c_str(), &backup_db) != SQLITE_OK) {
    LogWarning("Could not open backup database %s", backup_path.c_str());
    return;
  }
  sqlite3_backup *backup = sqlite3_backup_init(backup_db, "main", db_, "main");
  if (backup) {
    sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
  }
  sqlite3_close(backup_db);
  BackupFinished.Emit();
}

std::string Database::SchemaSql() { return LoadSchemaResource(); }

bool Database::Migrate() {
  Exec("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL)");
  int version = 0;
  sqlite3_stmt *stmt = nullptr;
  if (Prepare("SELECT version FROM schema_version LIMIT 1", &stmt)) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }
  startup_schema_version_ = version;
  if (version >= kSchemaVersion) {
    return true;
  }
  const std::string schema = SchemaSql();
  if (schema.empty()) {
    LogError("Embedded schema.sql is missing");
    return false;
  }
  if (!Exec(schema)) {
    return false;
  }
  LogInfo("Initialized collection database schema version %d", kSchemaVersion);
  return true;
}

SqlQuery::SqlQuery(Database *db, const std::string &sql) {
  if (db) {
    db->Prepare(sql, &stmt_);
  }
}

SqlQuery::~SqlQuery() {
  if (stmt_) {
    sqlite3_finalize(stmt_);
  }
}

void SqlQuery::Bind(int index, const std::string &value) {
  sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

void SqlQuery::Bind(int index, int value) { sqlite3_bind_int(stmt_, index, value); }

void SqlQuery::Bind(int index, int64_t value) { sqlite3_bind_int64(stmt_, index, value); }

void SqlQuery::Bind(int index, double value) { sqlite3_bind_double(stmt_, index, value); }

void SqlQuery::BindNull(int index) { sqlite3_bind_null(stmt_, index); }

bool SqlQuery::Step() { return stmt_ && sqlite3_step(stmt_) == SQLITE_ROW; }

bool SqlQuery::Exec() { return stmt_ && sqlite3_step(stmt_) == SQLITE_DONE; }

int SqlQuery::ColumnInt(int index) const { return sqlite3_column_int(stmt_, index); }

int64_t SqlQuery::ColumnInt64(int index) const { return sqlite3_column_int64(stmt_, index); }

double SqlQuery::ColumnDouble(int index) const { return sqlite3_column_double(stmt_, index); }

std::string SqlQuery::ColumnText(int index) const {
  const unsigned char *text = sqlite3_column_text(stmt_, index);
  return text ? reinterpret_cast<const char *>(text) : std::string();
}

bool SqlQuery::ColumnIsNull(int index) const { return sqlite3_column_type(stmt_, index) == SQLITE_NULL; }
