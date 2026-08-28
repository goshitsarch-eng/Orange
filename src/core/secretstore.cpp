#include "core/secretstore.h"

#include "core/logging.h"

#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif

namespace SecretStore {

#ifdef HAVE_LIBSECRET

namespace {

// One schema for every credential, with the settings group and key as its attributes.
const SecretSchema *Schema() {
  static const SecretSchema schema = {"org.strawberrymusicplayer.strawberry",
                                      SECRET_SCHEMA_NONE,
                                      {
                                          {"group", SECRET_SCHEMA_ATTRIBUTE_STRING},
                                          {"key", SECRET_SCHEMA_ATTRIBUTE_STRING},
                                          {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
                                      },
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0,
                                      0};
  return &schema;
}

std::string Label(const std::string &group, const std::string &key) { return "Orange: " + group + "/" + key; }

// Talking to the keyring can fail for reasons that are not this application's problem - no D-Bus, no keyring
// daemon, the user cancelled the unlock prompt. Report the first one and stay quiet after that, so a session
// with no keyring does not fill the log with one line per credential.
bool ReportError(const char *what, GError *error) {
  if (!error) {
    return false;
  }
  static bool reported = false;
  if (!reported) {
    reported = true;
    LogWarning("Secret store: could not %s: %s. Credentials will be kept in the settings file instead.", what, error->message);
  }
  g_error_free(error);
  return true;
}

}  // namespace

bool RealAvailable() {
  static const bool available = []() {
    GError *error = nullptr;
    // SECRET_SERVICE_OPEN_SESSION forces an actual session with the daemon.
    // Without it a proxy is handed back even when nothing owns the name, and every later call fails one by
    // one instead.
    SecretService *service = secret_service_get_sync(SECRET_SERVICE_OPEN_SESSION, nullptr, &error);
    if (ReportError("reach the keyring", error) || !service) {
      return false;
    }
    g_object_unref(service);
    return true;
  }();
  return available;
}

bool RealStore(const std::string &group, const std::string &key, const std::string &secret) {
  if (!RealAvailable()) {
    return false;
  }
  GError *error = nullptr;
  const gboolean ok = secret_password_store_sync(Schema(), SECRET_COLLECTION_DEFAULT, Label(group, key).c_str(), secret.c_str(), nullptr,
                                                 &error, "group", group.c_str(), "key", key.c_str(), nullptr);
  ReportError("store a credential", error);
  return ok == TRUE;
}

std::string RealLookup(const std::string &group, const std::string &key, bool *found) {
  if (found) {
    *found = false;
  }
  if (!RealAvailable()) {
    return {};
  }
  GError *error = nullptr;
  gchar *secret = secret_password_lookup_sync(Schema(), nullptr, &error, "group", group.c_str(), "key", key.c_str(), nullptr);
  ReportError("read a credential", error);
  if (!secret) {
    return {};
  }
  std::string result(secret);
  // Wipes the copy libsecret handed over rather than leaving it in freed heap.
  secret_password_free(secret);
  if (found) {
    *found = true;
  }
  return result;
}

bool RealErase(const std::string &group, const std::string &key) {
  if (!RealAvailable()) {
    return false;
  }
  GError *error = nullptr;
  const gboolean ok = secret_password_clear_sync(Schema(), nullptr, &error, "group", group.c_str(), "key", key.c_str(), nullptr);
  ReportError("remove a credential", error);
  return ok == TRUE;
}

#else  // !HAVE_LIBSECRET

bool RealAvailable() { return false; }

bool RealStore(const std::string &group, const std::string &key, const std::string &secret) {
  (void)group;
  (void)key;
  (void)secret;
  return false;
}

std::string RealLookup(const std::string &group, const std::string &key, bool *found) {
  (void)group;
  (void)key;
  if (found) {
    *found = false;
  }
  return {};
}

bool RealErase(const std::string &group, const std::string &key) {
  (void)group;
  (void)key;
  return false;
}

#endif  // HAVE_LIBSECRET

namespace {
Backend g_backend;
}  // namespace

void SetBackendForTesting(const Backend &backend) { g_backend = backend; }

bool Available() { return g_backend.available ? g_backend.available() : RealAvailable(); }

bool Store(const std::string &group, const std::string &key, const std::string &secret) {
  return g_backend.store ? g_backend.store(group, key, secret) : RealStore(group, key, secret);
}

std::string Lookup(const std::string &group, const std::string &key, bool *found) {
  return g_backend.lookup ? g_backend.lookup(group, key, found) : RealLookup(group, key, found);
}

bool Erase(const std::string &group, const std::string &key) {
  return g_backend.erase ? g_backend.erase(group, key) : RealErase(group, key);
}

}  // namespace SecretStore
