#include "qobuz/qobuzcredentialfetcher.h"

#include "core/settings.h"
#include "constants/qobuzsettings.h"

void QobuzCredentialFetcher::Fetch(NetworkAccessManager *, Callback callback) {
  Settings settings;
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  const std::string app_id = settings.Value(QobuzSettings::kAppId);
  const std::string app_secret = settings.Value(QobuzSettings::kAppSecret);
  if (callback) {
    callback(app_id, app_secret, app_id.empty() ? "Missing Qobuz app id" : std::string());
  }
}
