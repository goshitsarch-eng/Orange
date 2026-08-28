#include "core/secretstore.h"

#include "core/settings.h"
#include "core/standardpaths.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <map>
#include <string>
#include <sys/stat.h>

#include <gtest/gtest.h>

namespace {

// Stands in for the keyring, which needs a running daemon that tests cannot assume.
std::map<std::string, std::string> g_keyring;
bool g_keyring_available = true;

std::string KeyOf(const std::string &group, const std::string &key) { return group + "/" + key; }

SecretStore::Backend FakeBackend() {
  SecretStore::Backend backend;
  backend.available = []() { return g_keyring_available; };
  backend.store = [](const std::string &group, const std::string &key, const std::string &secret) {
    if (!g_keyring_available) {
      return false;
    }
    g_keyring[KeyOf(group, key)] = secret;
    return true;
  };
  backend.lookup = [](const std::string &group, const std::string &key, bool *found) {
    if (found) {
      *found = false;
    }
    if (!g_keyring_available) {
      return std::string();
    }
    const auto it = g_keyring.find(KeyOf(group, key));
    if (it == g_keyring.end()) {
      return std::string();
    }
    if (found) {
      *found = true;
    }
    return it->second;
  };
  backend.erase = [](const std::string &group, const std::string &key) {
    return g_keyring.erase(KeyOf(group, key)) > 0;
  };
  return backend;
}

// The test main points the XDG directories at a scratch tree, so Settings reads and writes there.
// Each test starts from a clean settings file and an empty fake keyring.
class SecretSettingsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_keyring.clear();
    g_keyring_available = true;
    SecretStore::SetBackendForTesting(FakeBackend());
    g_remove(SettingsPath().c_str());
  }

  void TearDown() override {
    SecretStore::SetBackendForTesting(SecretStore::Backend{});
    g_remove(SettingsPath().c_str());
  }

  // Writes a settings file the way a configuration from before the keyring existed would look.
  void WritePlaintextSettings(const std::string &group, const std::string &key, const std::string &value) const {
    GKeyFile *file = g_key_file_new();
    g_key_file_set_string(file, group.c_str(), key.c_str(), value.c_str());
    const std::string path = SettingsPath();
    gchar *dir = g_path_get_dirname(path.c_str());
    g_mkdir_with_parents(dir, 0700);
    g_free(dir);
    ASSERT_TRUE(g_key_file_save_to_file(file, path.c_str(), nullptr));
    g_key_file_unref(file);
  }

  std::string SettingsPath() const { return StandardPaths::SettingsPath(); }

  std::string RawSettingsContents() const {
    gchar *contents = nullptr;
    if (!g_file_get_contents(SettingsPath().c_str(), &contents, nullptr, nullptr)) {
      return {};
    }
    std::string result = contents ? contents : "";
    g_free(contents);
    return result;
  }

};

}  // namespace

TEST_F(SecretSettingsTest, SecretGoesToTheKeyringAndNotTheSettingsFile) {
  {
    Settings settings;
    settings.BeginGroup("Subsonic");
    settings.SetSecretValue("password", "hunter2");
    settings.Sync();
  }
  EXPECT_EQ("hunter2", g_keyring["Subsonic/password"]);
  EXPECT_EQ(RawSettingsContents().find("hunter2"), std::string::npos) << RawSettingsContents();
}

TEST_F(SecretSettingsTest, SecretReadsBackFromTheKeyring) {
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetSecretValue("password", "hunter2");
  EXPECT_EQ("hunter2", settings.SecretValue("password"));
}

TEST_F(SecretSettingsTest, ExistingPlaintextIsMigratedAndRemoved) {
  WritePlaintextSettings("Subsonic", "password", "old-plaintext");
  {
    Settings settings;
    settings.BeginGroup("Subsonic");
    // Reading is what triggers the migration, so an existing configuration moves over on first use.
    EXPECT_EQ("old-plaintext", settings.SecretValue("password"));
    settings.Sync();
  }
  EXPECT_EQ("old-plaintext", g_keyring["Subsonic/password"]);
  EXPECT_EQ(RawSettingsContents().find("old-plaintext"), std::string::npos) << RawSettingsContents();
}

TEST_F(SecretSettingsTest, KeyringValueWinsOverAStaleSettingsFileEntry) {
  WritePlaintextSettings("Subsonic", "password", "stale");
  g_keyring["Subsonic/password"] = "current";
  Settings settings;
  settings.BeginGroup("Subsonic");
  EXPECT_EQ("current", settings.SecretValue("password"));
}

TEST_F(SecretSettingsTest, RemovingASecretClearsBothPlaces) {
  WritePlaintextSettings("Subsonic", "password", "old-plaintext");
  {
    Settings settings;
    settings.BeginGroup("Subsonic");
    settings.SetSecretValue("password", "hunter2");
    settings.RemoveSecret("password");
    settings.Sync();
  }
  EXPECT_EQ(g_keyring.count("Subsonic/password"), 0u);
  EXPECT_EQ(RawSettingsContents().find("hunter2"), std::string::npos) << RawSettingsContents();
  EXPECT_EQ(RawSettingsContents().find("old-plaintext"), std::string::npos) << RawSettingsContents();
}

TEST_F(SecretSettingsTest, SecretsAreSeparatedByGroup) {
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetSecretValue("password", "subsonic-secret");
  settings.BeginGroup("Tidal");
  settings.SetSecretValue("password", "tidal-secret");
  EXPECT_EQ("tidal-secret", settings.SecretValue("password"));
  settings.BeginGroup("Subsonic");
  EXPECT_EQ("subsonic-secret", settings.SecretValue("password"));
}

// Without a keyring the settings file has to stay a working fallback, or credentials would be lost.
TEST_F(SecretSettingsTest, FallsBackToTheSettingsFileWithNoKeyring) {
  g_keyring_available = false;
  {
    Settings settings;
    settings.BeginGroup("Subsonic");
    settings.SetSecretValue("password", "hunter2");
    settings.Sync();
  }
  EXPECT_NE(RawSettingsContents().find("hunter2"), std::string::npos) << RawSettingsContents();
  Settings settings;
  settings.BeginGroup("Subsonic");
  EXPECT_EQ("hunter2", settings.SecretValue("password"));
}

// The settings file still holds account names and server addresses, and on a build with no keyring the
// credentials themselves, so it must not be left world-readable.
TEST_F(SecretSettingsTest, SettingsFileIsOwnerOnly) {
  {
    Settings settings;
    settings.BeginGroup("Subsonic");
    settings.SetValue("username", "alice");
    settings.Sync();
  }
  struct stat info {};
  ASSERT_EQ(0, stat(SettingsPath().c_str(), &info));
  EXPECT_EQ(0, info.st_mode & (S_IRWXG | S_IRWXO)) << "mode is " << std::oct << (info.st_mode & 07777);
}
