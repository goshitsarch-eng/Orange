#ifndef STRAWBERRY_SECRETSTORE_H
#define STRAWBERRY_SECRETSTORE_H

#include <string>

// Storage for credentials - service passwords, OAuth tokens and scrobbler session keys.
//
// When the build has libsecret these go to the login keyring, so they never sit in the settings file.
// Without libsecret every call reports failure and the callers fall back to the settings file, which is
// where the credentials used to live unconditionally.
namespace SecretStore {

// Whether a keyring is actually usable right now.
// False when the build has no libsecret, and also when there is no session keyring to talk to (a headless
// session, a locked keyring, no D-Bus).
bool Available();

// The three operations are keyed by the settings group and key the credential belongs to, so a credential
// keeps the same identity it had in the settings file.
bool Store(const std::string &group, const std::string &key, const std::string &secret);

// Returns the stored secret, or an empty string if there is none.
// `found` distinguishes "stored as empty" from "not stored", which the migration path needs.
std::string Lookup(const std::string &group, const std::string &key, bool *found = nullptr);

bool Erase(const std::string &group, const std::string &key);

// Test seam.
// The real backend needs a running keyring daemon, which tests cannot assume, so they substitute one and
// restore the default afterwards by installing a default-constructed Backend.
struct Backend {
  bool (*available)() = nullptr;
  bool (*store)(const std::string &group, const std::string &key, const std::string &secret) = nullptr;
  std::string (*lookup)(const std::string &group, const std::string &key, bool *found) = nullptr;
  bool (*erase)(const std::string &group, const std::string &key) = nullptr;
};

void SetBackendForTesting(const Backend &backend);

}  // namespace SecretStore

#endif  // STRAWBERRY_SECRETSTORE_H
