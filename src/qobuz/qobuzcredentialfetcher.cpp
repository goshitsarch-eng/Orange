#include "qobuz/qobuzcredentialfetcher.h"

#include "qobuz/qobuzcredentialparser.h"

void QobuzCredentialFetcher::Fetch(NetworkAccessManager *network, Callback callback) {
  if (!callback) {
    return;
  }
  if (!network) {
    callback({}, {}, {}, "Network is unavailable.");
    return;
  }
  const std::map<std::string, std::string> headers = {{"User-Agent", QobuzCredentialParser::UserAgent()}};
  network->Get(QobuzCredentialParser::LoginPageUrl(), [network, callback, headers](const NetworkAccessManager::Response &login) {
    if (!login.ok()) {
      callback({}, {}, {}, QobuzCredentialParser::FailedLoginPage(login.error.empty() ? std::to_string(login.status) : login.error));
      return;
    }
    const std::string path = QobuzCredentialParser::ExtractBundlePath(login.body);
    if (path.empty()) {
      callback({}, {}, {}, QobuzCredentialParser::MissingBundle());
      return;
    }
    network->Get(QobuzCredentialParser::BundleUrl(path), [callback](const NetworkAccessManager::Response &bundle) {
      if (!bundle.ok()) {
        callback({}, {}, {}, QobuzCredentialParser::FailedBundle(bundle.error.empty() ? std::to_string(bundle.status) : bundle.error));
        return;
      }
      const std::string app_id = QobuzCredentialParser::ExtractAppId(bundle.body);
      if (app_id.empty()) {
        callback({}, {}, {}, QobuzCredentialParser::MissingAppId());
        return;
      }
      const std::string app_secret = QobuzCredentialParser::ExtractAppSecret(bundle.body);
      if (app_secret.empty()) {
        callback({}, {}, {}, QobuzCredentialParser::MissingAppSecret());
        return;
      }
      callback(app_id, app_secret, QobuzCredentialParser::ExtractPrivateKey(bundle.body), {});
    }, headers);
  }, headers);
}
