#include "tagfetcher/acoustidclient.h"

#include "core/networktimeoutpolicy.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>
#include <glib.h>

AcoustidClient::AcoustidClient(NetworkAccessManager *network) : network_(network) {
  timeouts_.SetTimeout(NetworkTimeoutPolicy::kAcoustidTimeoutMs);
  timeouts_.SetAbort([this](int req_id) {
    if (network_) {
      network_->Cancel(req_id);
    }
  });
}

std::vector<std::string> AcoustidClient::ParseMbids(const std::string &json) {
  std::vector<std::string> mbids;
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.c_str(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return mbids;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return mbids;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "results") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "results"))) {
    g_object_unref(parser);
    return mbids;
  }
  JsonArray *results = json_object_get_array_member(object, "results");
  const guint n = json_array_get_length(results);
  for (guint i = 0; i < n; ++i) {
    JsonObject *result = json_array_get_object_element(results, i);
    if (!result || !json_object_has_member(result, "recordings") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(result, "recordings"))) {
      continue;
    }
    JsonArray *recordings = json_object_get_array_member(result, "recordings");
    const guint rec_n = json_array_get_length(recordings);
    for (guint j = 0; j < rec_n; ++j) {
      JsonObject *recording = json_array_get_object_element(recordings, j);
      if (recording && json_object_has_member(recording, "id")) {
        const char *id = json_object_get_string_member(recording, "id");
        if (id) {
          mbids.emplace_back(id);
        }
      }
    }
  }
  g_object_unref(parser);
  return mbids;
}

void AcoustidClient::Start(int id, const std::string &fingerprint, int duration_msec) {
  if (!network_ || fingerprint.empty()) {
    Finished.Emit(id, {}, "Missing fingerprint");
    return;
  }
  gchar *escaped = g_uri_escape_string(fingerprint.c_str(), nullptr, TRUE);
  const std::string url = std::string("https://api.acoustid.org/v2/lookup?client=strawberry&meta=recordingids&duration=") +
                          std::to_string(std::max(1, duration_msec / 1000)) + "&fingerprint=" + (escaped ? escaped : "");
  g_free(escaped);
  timeouts_.SetTimeout(timeout_msec_);
  const int req = network_->Get(url, [this, id](const NetworkAccessManager::Response &response) {
    auto it = requests_.find(id);
    if (it != requests_.end()) {
      timeouts_.Cancel(it->second);
      requests_.erase(it);
    }
    if (!response.ok()) {
      Finished.Emit(id, {}, NetworkTimeoutPolicy::FailureMessage(response.error, "AcoustID request failed"));
      return;
    }
    Finished.Emit(id, ParseMbids(response.body), {});
  });
  requests_[id] = req;
  timeouts_.AddReply(req);
}

void AcoustidClient::Cancel(int id) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  timeouts_.Cancel(it->second);
  if (network_) {
    network_->Cancel(it->second);
  }
  requests_.erase(it);
}

void AcoustidClient::CancelAll() {
  for (const auto &entry : requests_) {
    timeouts_.Cancel(entry.second);
    if (network_) {
      network_->Cancel(entry.second);
    }
  }
  requests_.clear();
}
