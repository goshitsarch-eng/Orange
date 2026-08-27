#include "core/database.h"
#include "core/databaseschema.h"
#include "dialogs/consolequery.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <unistd.h>

#include <string>
#include <vector>

TEST(Database, OpenCreatesFile) {
  const std::string path = "/tmp/strawberry-db-test-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  sqlite3 *handle = nullptr;
  ASSERT_EQ(SQLITE_OK, sqlite3_open(path.c_str(), &handle));
  ASSERT_TRUE(handle);
  char *error = nullptr;
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle, "CREATE TABLE t(id INTEGER); INSERT INTO t VALUES (1);", nullptr, nullptr, &error));
  sqlite3_close(handle);
  unlink(path.c_str());
}

TEST(ConsoleQuery, SelectFormatsRowsAndReportsErrors) {
  const std::string path = "/tmp/strawberry-console-test-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  ASSERT_TRUE(db.Exec("CREATE TABLE IF NOT EXISTS console_test(id INTEGER, name TEXT)"));
  const auto insert = ConsoleQuery::Run(&db, "INSERT INTO console_test VALUES (1, 'Roads')");
  EXPECT_TRUE(insert.ok);
  EXPECT_TRUE(insert.rows.empty());
  const auto rows = ConsoleQuery::Run(&db, "SELECT id, name FROM console_test");
  ASSERT_TRUE(rows.ok);
  ASSERT_EQ(1u, rows.rows.size());
  EXPECT_EQ("1|Roads", ConsoleQuery::JoinRow(rows.rows.front()));
  EXPECT_EQ("id|name", ConsoleQuery::JoinRow(rows.columns));
  const std::string formatted = ConsoleQuery::Format(rows);
  EXPECT_NE(formatted.find("> SELECT id, name FROM console_test"), std::string::npos);
  EXPECT_NE(formatted.find("1|Roads"), std::string::npos);
  const auto missing = ConsoleQuery::Run(&db, "SELECT * FROM not_a_table");
  EXPECT_FALSE(missing.ok);
  EXPECT_FALSE(missing.error.empty());
  EXPECT_NE(ConsoleQuery::Format(missing).find(missing.error), std::string::npos);
  EXPECT_EQ("no database", ConsoleQuery::Run(nullptr, "SELECT 1").error);
  unlink(path.c_str());
}

TEST(DatabaseSchema, ResourcePathsMatchGResource) {
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/schema/schema.sql", DatabaseSchema::ResourcePath(0));
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/schema/schema-10.sql", DatabaseSchema::ResourcePath(10));
  EXPECT_EQ("/org/strawberrymusicplayer/Strawberry/schema/schema-23.sql", DatabaseSchema::ResourcePath(23));
}

TEST(DatabaseSchema, IncrementalVersionsMatchQtLoop) {
  EXPECT_TRUE(DatabaseSchema::IsEmptyDatabase(0));
  EXPECT_FALSE(DatabaseSchema::IsEmptyDatabase(1));
  EXPECT_TRUE(DatabaseSchema::IsSupported(10));
  EXPECT_FALSE(DatabaseSchema::IsSupported(9));
  const std::vector<int> versions = DatabaseSchema::IncrementalVersions(20, 23);
  ASSERT_EQ(3u, versions.size());
  EXPECT_EQ(21, versions[0]);
  EXPECT_EQ(22, versions[1]);
  EXPECT_EQ(23, versions[2]);
  EXPECT_TRUE(DatabaseSchema::IncrementalVersions(23, 23).empty());
}

TEST(DatabaseSchema, SplitsCommandsAndExpandsMagicTables) {
  const std::string sql = "ALTER TABLE %allsongstables ADD COLUMN rating INTEGER DEFAULT -1;\n\n"
                          "UPDATE schema_version SET version=13;\n";
  const std::vector<std::string> commands = DatabaseSchema::SplitCommands(sql);
  ASSERT_EQ(2u, commands.size());
  EXPECT_EQ("ALTER TABLE %allsongstables ADD COLUMN rating INTEGER DEFAULT -1", commands[0]);
  EXPECT_EQ("UPDATE schema_version SET version=13;", commands[1]);
  const std::vector<std::string> tables = DatabaseSchema::SongsTables({"directories", "songs", "subsonic_songs", "playlists"});
  ASSERT_EQ(3u, tables.size());
  EXPECT_EQ("songs", tables[0]);
  EXPECT_EQ("subsonic_songs", tables[1]);
  EXPECT_EQ("playlist_items", tables[2]);
  const std::vector<std::string> expanded = DatabaseSchema::ExpandCommand(commands[0], tables);
  ASSERT_EQ(3u, expanded.size());
  EXPECT_EQ("ALTER TABLE songs ADD COLUMN rating INTEGER DEFAULT -1", expanded[0]);
  EXPECT_EQ("ALTER TABLE subsonic_songs ADD COLUMN rating INTEGER DEFAULT -1", expanded[1]);
  EXPECT_EQ("ALTER TABLE playlist_items ADD COLUMN rating INTEGER DEFAULT -1", expanded[2]);
  EXPECT_EQ("DROP TABLE IF EXISTS songs_fts", DatabaseSchema::ExpandMagic("DROP TABLE IF EXISTS %allsongstables_fts", "songs"));
}

namespace {

bool ColumnExists(sqlite3 *db, const char *table, const char *column) {
  sqlite3_stmt *stmt = nullptr;
  const std::string sql = std::string("SELECT 1 FROM pragma_table_info('") + table + "') WHERE name = ?";
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }
  sqlite3_bind_text(stmt, 1, column, -1, SQLITE_TRANSIENT);
  const bool found = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return found;
}

int SchemaVersion(sqlite3 *db) {
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT version FROM schema_version LIMIT 1", -1, &stmt, nullptr) != SQLITE_OK) {
    return -1;
  }
  int version = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    version = sqlite3_column_int(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return version;
}

bool IndexExists(sqlite3 *db, const char *name) {
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type = 'index' AND name = ?", -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }
  sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
  const bool found = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return found;
}

const char *kLegacySongsDdl =
    "CREATE TABLE schema_version (version INTEGER NOT NULL);"
    "CREATE TABLE songs ("
    "  title TEXT,"
    "  album TEXT,"
    "  url TEXT NOT NULL,"
    "  beginning INTEGER NOT NULL DEFAULT 0,"
    "  unavailable INTEGER DEFAULT 0,"
    "  playcount INTEGER NOT NULL DEFAULT 0,"
    "  skipcount INTEGER NOT NULL DEFAULT 0,"
    "  lastplayed INTEGER NOT NULL DEFAULT -1,"
    "  lastseen INTEGER NOT NULL DEFAULT -1,"
    "  rating INTEGER DEFAULT -1"
    ");"
    "CREATE TABLE playlist_items ("
    "  playlist INTEGER NOT NULL,"
    "  type INTEGER NOT NULL DEFAULT 0,"
    "  collection_id INTEGER"
    ");";

}  // namespace

TEST(Database, AppliesIncrementalSchemaFromTwenty) {
  const std::string path = "/tmp/strawberry-schema-from-20-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  sqlite3 *handle = nullptr;
  ASSERT_EQ(SQLITE_OK, sqlite3_open(path.c_str(), &handle));
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle, kLegacySongsDdl, nullptr, nullptr, nullptr));
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle, "INSERT INTO schema_version (version) VALUES (20);", nullptr, nullptr, nullptr));
  sqlite3_close(handle);

  Database db(path);
  ASSERT_TRUE(db.Open());
  EXPECT_EQ(20, db.startup_schema_version());
  EXPECT_EQ(Database::kCurrentSchemaVersion, db.current_schema_version());
  EXPECT_TRUE(ColumnExists(db.handle(), "songs", "albumartistsort"));
  EXPECT_TRUE(ColumnExists(db.handle(), "songs", "bpm"));
  EXPECT_TRUE(ColumnExists(db.handle(), "playlist_items", "uuid"));
  EXPECT_TRUE(IndexExists(db.handle(), "idx_songs_url_beginning"));
  EXPECT_EQ(Database::kCurrentSchemaVersion, SchemaVersion(db.handle()));
  db.Close();
  unlink(path.c_str());
}

TEST(Database, DeduplicatesSongsOnSchemaTwentyThree) {
  const std::string path = "/tmp/strawberry-schema-from-22-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  sqlite3 *handle = nullptr;
  ASSERT_EQ(SQLITE_OK, sqlite3_open(path.c_str(), &handle));
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle, kLegacySongsDdl, nullptr, nullptr, nullptr));
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle,
                                    "ALTER TABLE playlist_items ADD COLUMN uuid TEXT;"
                                    "INSERT INTO schema_version (version) VALUES (22);"
                                    "INSERT INTO songs (title, url, beginning, unavailable, playcount, skipcount, lastplayed, lastseen, rating) "
                                    "VALUES ('Keep', 'file:///tmp/a.flac', 0, 0, 3, 1, 100, 50, 4);"
                                    "INSERT INTO songs (title, url, beginning, unavailable, playcount, skipcount, lastplayed, lastseen, rating) "
                                    "VALUES ('Dup', 'file:///tmp/a.flac', 0, 1, 2, 1, 80, 40, 2);",
                                    nullptr, nullptr, nullptr));
  sqlite3_close(handle);

  Database db(path);
  ASSERT_TRUE(db.Open());
  EXPECT_EQ(22, db.startup_schema_version());
  SqlQuery count(&db, "SELECT COUNT(*), SUM(playcount), MAX(rating) FROM songs");
  ASSERT_TRUE(count.Step());
  EXPECT_EQ(1, count.ColumnInt(0));
  EXPECT_EQ(5, count.ColumnInt(1));
  EXPECT_EQ(4, count.ColumnInt(2));
  EXPECT_TRUE(IndexExists(db.handle(), "idx_songs_url_beginning"));
  db.Close();
  unlink(path.c_str());
}

TEST(Database, EmitsErrorOnSqlFailure) {
  const std::string path = "/tmp/strawberry-db-error-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  std::string error;
  db.Error.Connect([&error](const std::string &text) { error = text; });
  EXPECT_FALSE(db.Exec("THIS IS NOT SQL"));
  EXPECT_FALSE(error.empty());
  unlink(path.c_str());
}

TEST(Database, RejectsSchemaOlderThanMinimum) {
  const std::string path = "/tmp/strawberry-schema-too-old-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  sqlite3 *handle = nullptr;
  ASSERT_EQ(SQLITE_OK, sqlite3_open(path.c_str(), &handle));
  ASSERT_EQ(SQLITE_OK, sqlite3_exec(handle,
                                    "CREATE TABLE schema_version (version INTEGER NOT NULL);"
                                    "INSERT INTO schema_version (version) VALUES (9);"
                                    "CREATE TABLE leftover (id INTEGER);",
                                    nullptr, nullptr, nullptr));
  sqlite3_close(handle);

  Database db(path);
  EXPECT_FALSE(db.Open());
  unlink(path.c_str());
}
