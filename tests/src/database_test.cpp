#include "core/database.h"
#include "dialogs/consolequery.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <unistd.h>

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
