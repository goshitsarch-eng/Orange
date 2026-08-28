#ifndef STRAWBERRY_SETTINGS_H
#define STRAWBERRY_SETTINGS_H

#include <glib.h>
#include <string>
#include <vector>

class Settings {
 public:
  Settings();
  ~Settings();

  Settings(const Settings &) = delete;
  Settings &operator=(const Settings &) = delete;

  void BeginGroup(const std::string &group);
  void EndGroup();

  bool Contains(const std::string &key) const;

  std::string Value(const std::string &key, const std::string &fallback = {}) const;
  int IntValue(const std::string &key, int fallback = 0) const;
  gint64 Int64Value(const std::string &key, gint64 fallback = 0) const;
  double DoubleValue(const std::string &key, double fallback = 0.0) const;
  bool BoolValue(const std::string &key, bool fallback = false) const;

  void SetValue(const std::string &key, const std::string &value);
  void SetIntValue(const std::string &key, int value);
  void SetInt64Value(const std::string &key, gint64 value);
  void SetDoubleValue(const std::string &key, double value);
  void SetBoolValue(const std::string &key, bool value);

  // Credentials - passwords, OAuth tokens, scrobbler session keys.
  // These go to the system keyring when one is available, and fall back to the settings file when it is
  // not. Reading migrates any value still held in the settings file into the keyring and drops the
  // plaintext copy, so an existing configuration moves over on first use.
  std::string SecretValue(const std::string &key, const std::string &fallback = {});
  void SetSecretValue(const std::string &key, const std::string &value);
  void RemoveSecret(const std::string &key);

  void Remove(const std::string &key);
  std::vector<std::string> Keys() const;
  bool Sync();

  static Settings &Instance();

 private:
  std::string FullKey(const std::string &key) const;

  GKeyFile *key_file_;
  std::string path_;
  std::string group_;
  // Whether anything has been written since the last save.
  // Settings objects are constructed all over the place purely to read a value, and rewriting the whole
  // file when each of those is destroyed made reads as expensive as writes.
  bool dirty_ = false;
};

#endif
