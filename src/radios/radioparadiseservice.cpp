#include "radios/radioparadiseservice.h"

#include <json-glib/json-glib.h>

#include <cctype>

const char *RadioParadiseService::kApiChannelsUrl = "https://api.radioparadise.com/api/list_streams";

std::string RadioParadiseService::Homepage() { return "https://radioparadise.com/"; }

std::string RadioParadiseService::Donate() {
  return "https://payments.radioparadise.com/rp2s-content.php?name=Support&file=support";
}

std::string RadioParadiseService::EnsureAbsoluteUrl(const std::string &url) {
  if (url.empty()) {
    return {};
  }
  size_t scheme = 0;
  while (scheme < url.size() && std::isalnum(static_cast<unsigned char>(url[scheme]))) {
    ++scheme;
  }
  if (scheme > 0 && scheme + 2 < url.size() && url[scheme] == ':' && url[scheme + 1] == '/' && url[scheme + 2] == '/') {
    return url;
  }
  return "https://" + url;
}

std::vector<RadioChannel> RadioParadiseService::ParseChannels(const std::string &json) {
  std::vector<RadioChannel> channels;
  if (json.empty()) {
    return channels;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return channels;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return channels;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "channels") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "channels"))) {
    g_object_unref(parser);
    return channels;
  }
  JsonArray *array = json_object_get_array_member(object, "channels");
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *channel_obj = json_node_get_object(item);
    if (!json_object_has_member(channel_obj, "chan_name") || !json_object_has_member(channel_obj, "streams") ||
        !JSON_NODE_HOLDS_ARRAY(json_object_get_member(channel_obj, "streams"))) {
      continue;
    }
    const char *name = json_object_get_string_member(channel_obj, "chan_name");
    if (!name) {
      continue;
    }
    JsonArray *streams = json_object_get_array_member(channel_obj, "streams");
    const guint stream_count = json_array_get_length(streams);
    for (guint s = 0; s < stream_count; ++s) {
      JsonNode *stream_node = json_array_get_element(streams, s);
      if (!stream_node || !JSON_NODE_HOLDS_OBJECT(stream_node)) {
        continue;
      }
      JsonObject *stream = json_node_get_object(stream_node);
      if (!json_object_has_member(stream, "label") || !json_object_has_member(stream, "url")) {
        continue;
      }
      const char *label = json_object_get_string_member(stream, "label");
      const char *url = json_object_get_string_member(stream, "url");
      if (!label || !url) {
        continue;
      }
      RadioChannel channel;
      channel.source = Song::Source::RadioParadise;
      channel.name = std::string(name) + " - " + label;
      channel.url = EnsureAbsoluteUrl(url);
      channels.push_back(channel);
    }
  }
  g_object_unref(parser);
  return channels;
}
