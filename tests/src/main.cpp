#include <glib.h>

#include <gtest/gtest.h>

int main(int argc, char **argv) {
  // Point the XDG user directories at a scratch tree before anything asks GLib for them.
  // GLib caches those directories on first use, so this has to happen before any test touches Settings or
  // StandardPaths; otherwise tests would read and write the real configuration of whoever runs them.
  gchar *sandbox = g_dir_make_tmp("strawberry-tests-XXXXXX", nullptr);
  if (sandbox) {
    gchar *config = g_build_filename(sandbox, "config", nullptr);
    gchar *data = g_build_filename(sandbox, "data", nullptr);
    gchar *cache = g_build_filename(sandbox, "cache", nullptr);
    g_mkdir_with_parents(config, 0700);
    g_mkdir_with_parents(data, 0700);
    g_mkdir_with_parents(cache, 0700);
    g_setenv("HOME", sandbox, TRUE);
    g_setenv("XDG_CONFIG_HOME", config, TRUE);
    g_setenv("XDG_DATA_HOME", data, TRUE);
    g_setenv("XDG_CACHE_HOME", cache, TRUE);
    g_free(config);
    g_free(data);
    g_free(cache);
    g_free(sandbox);
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
