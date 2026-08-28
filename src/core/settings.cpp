#include "core/settings.h"

#include "core/logging.h"
#include "core/secretstore.h"
#include "core/standardpaths.h"

#include <glib/gstdio.h>

#include <memory>

namespace {
Settings *g_instance = nullptr;
}

Settings::Settings() : key_file_(g_key_file_new()), path_(StandardPaths::SettingsPath()), group_("General") {
  GError *error = nullptr;
  if (!g_key_file_load_from_file(key_file_, path_.c_str(), static_cast<GKeyFileFlags>(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS), &error)) {
    if (error) {
      if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
        LogWarning("Could not load settings from %s: %s", path_.c_str(), error->message);
      }
      g_error_free(error);
    }
  }
}

Settings::~Settings() {
  // Only write if this object actually changed something.
  if (dirty_) {
    Sync();
  }
  g_key_file_unref(key_file_);
  if (g_instance == this) {
    g_instance = nullptr;
  }
}

Settings &Settings::Instance() {
  if (!g_instance) {
    g_instance = new Settings();
  }
  return *g_instance;
}

void Settings::BeginGroup(const std::string &group) { group_ = group.empty() ? "General" : group; }

void Settings::EndGroup() { group_ = "General"; }

std::string Settings::FullKey(const std::string &key) const { return key; }

bool Settings::Contains(const std::string &key) const {
  return g_key_file_has_key(key_file_, group_.c_str(), key.c_str(), nullptr);
}

std::string Settings::Value(const std::string &key, const std::string &fallback) const {
  GError *error = nullptr;
  gchar *value = g_key_file_get_string(key_file_, group_.c_str(), key.c_str(), &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  std::string result = value ? value : fallback;
  g_free(value);
  return result;
}

int Settings::IntValue(const std::string &key, int fallback) const {
  GError *error = nullptr;
  const int value = g_key_file_get_integer(key_file_, group_.c_str(), key.c_str(), &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

gint64 Settings::Int64Value(const std::string &key, gint64 fallback) const {
  GError *error = nullptr;
  const gint64 value = g_key_file_get_int64(key_file_, group_.c_str(), key.c_str(), &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

double Settings::DoubleValue(const std::string &key, double fallback) const {
  GError *error = nullptr;
  const double value = g_key_file_get_double(key_file_, group_.c_str(), key.c_str(), &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

bool Settings::BoolValue(const std::string &key, bool fallback) const {
  GError *error = nullptr;
  const gboolean value = g_key_file_get_boolean(key_file_, group_.c_str(), key.c_str(), &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

void Settings::SetValue(const std::string &key, const std::string &value) {
  g_key_file_set_string(key_file_, group_.c_str(), key.c_str(), value.c_str());
  dirty_ = true;
}

void Settings::SetIntValue(const std::string &key, int value) {
  g_key_file_set_integer(key_file_, group_.c_str(), key.c_str(), value);
  dirty_ = true;
}

void Settings::SetInt64Value(const std::string &key, gint64 value) {
  g_key_file_set_int64(key_file_, group_.c_str(), key.c_str(), value);
  dirty_ = true;
}

void Settings::SetDoubleValue(const std::string &key, double value) {
  g_key_file_set_double(key_file_, group_.c_str(), key.c_str(), value);
  dirty_ = true;
}

void Settings::SetBoolValue(const std::string &key, bool value) {
  g_key_file_set_boolean(key_file_, group_.c_str(), key.c_str(), value);
  dirty_ = true;
}

void Settings::Remove(const std::string &key) {
  g_key_file_remove_key(key_file_, group_.c_str(), key.c_str(), nullptr);
  dirty_ = true;
}

std::vector<std::string> Settings::Keys() const {
  std::vector<std::string> keys;
  gsize length = 0;
  gchar **names = g_key_file_get_keys(key_file_, group_.c_str(), &length, nullptr);
  if (!names) {
    return keys;
  }
  for (gsize i = 0; i < length; ++i) {
    if (names[i]) {
      keys.emplace_back(names[i]);
    }
  }
  g_strfreev(names);
  return keys;
}

std::string Settings::SecretValue(const std::string &key, const std::string &fallback) {
  bool found = false;
  const std::string stored = SecretStore::Lookup(group_, key, &found);
  if (found) {
    // A value still sitting in the settings file is stale once the keyring holds one.
    if (Contains(key)) {
      Remove(key);
      Sync();
    }
    return stored;
  }
  if (!Contains(key)) {
    return fallback;
  }
  const std::string plaintext = Value(key, fallback);
  // Migration: move what the settings file holds into the keyring and stop keeping it in the clear.
  if (!plaintext.empty() && SecretStore::Store(group_, key, plaintext)) {
    Remove(key);
    Sync();
  }
  return plaintext;
}

void Settings::SetSecretValue(const std::string &key, const std::string &value) {
  if (SecretStore::Store(group_, key, value)) {
    if (Contains(key)) {
      Remove(key);
    }
    return;
  }
  // No keyring available, so the settings file stays the only place to put this.
  SetValue(key, value);
}

void Settings::RemoveSecret(const std::string &key) {
  SecretStore::Erase(group_, key);
  Remove(key);
}

bool Settings::Sync() {
  GError *error = nullptr;
  if (!g_key_file_save_to_file(key_file_, path_.c_str(), &error)) {
    if (error) {
      LogWarning("Could not save settings to %s: %s", path_.c_str(), error->message);
      g_error_free(error);
    }
    return false;
  }
  dirty_ = false;
  // The file is created with the process umask, which normally leaves it world-readable. It holds account
  // names and server addresses, and on a build with no keyring it still holds the credentials themselves,
  // so restrict it to the owner.
  if (g_chmod(path_.c_str(), 0600) != 0) {
    LogWarning("Could not restrict permissions on %s", path_.c_str());
  }
  return true;
}
