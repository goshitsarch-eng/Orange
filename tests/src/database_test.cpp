#include "core/database.h"

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
