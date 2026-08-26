#ifndef STRAWBERRY_SUBSONICPING_H
#define STRAWBERRY_SUBSONICPING_H

#include "settings/streamingsettingslabels.h"
#include "subsonic/subsonicservice.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <string>

namespace SubsonicPing {

enum class Status { Ok, Failed, Invalid };

struct Result {
  Status status = Status::Invalid;
  std::string message;
  int code = 0;
};

inline std::string Url(const std::string &server_url, const std::string &username, const std::string &password, bool hex_auth) {
  return SubsonicService::CreateUrl(server_url, username, password, "ping", {}, hex_auth);
}

inline Result Parse(const std::string &json, unsigned http_status = 200, const std::string &http_error = {}) {
  Result result;
  const std::string status = StrUtils::ToLower(JsonUtils::GetString(json, {"subsonic-response", "status"}));
  result.code = JsonUtils::GetInt(json, {"subsonic-response", "error", "code"});
  const std::string error = JsonUtils::GetString(json, {"subsonic-response", "error", "message"});
  if (status == "ok") {
    result.status = Status::Ok;
    result.message = SubsonicSettingsLabels::TestSuccessful();
    return result;
  }
  if (status == "failed") {
    result.status = Status::Failed;
    result.message = error.empty() ? SubsonicSettingsLabels::TestFailed() : error;
    if (result.code > 0) {
      result.message += " (" + std::to_string(result.code) + ")";
    }
    return result;
  }
  if (!http_error.empty() && json.empty()) {
    result.status = Status::Failed;
    result.message = http_error;
    return result;
  }
  if (http_status < 200 || http_status >= 300) {
    result.status = Status::Failed;
    result.message = "Received HTTP code " + std::to_string(http_status);
    return result;
  }
  result.status = Status::Invalid;
  result.message = status.empty() ? "Ping reply from server is missing status" : "Ping reply status from server is unknown";
  return result;
}

inline const char *Title(Status status) {
  switch (status) {
    case Status::Ok:
      return SubsonicSettingsLabels::TestSuccessful();
    case Status::Failed:
    case Status::Invalid:
      return SubsonicSettingsLabels::TestFailed();
  }
  return SubsonicSettingsLabels::TestFailed();
}

inline const char *Title(const Result &result) { return Title(result.status); }

inline std::string Body(const Result &result) { return result.message.empty() ? Title(result.status) : result.message; }

}  // namespace SubsonicPing

#endif
