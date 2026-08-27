#include "radios/somafmservice.h"

#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

const char *SomaFMService::kApiChannelsUrl = "https://somafm.com/channels.json";
const char *SomaFMService::kQualityDefault = "highest";

std::string SomaFMService::Homepage() { return "https://somafm.com/"; }

std::string SomaFMService::Donate() { return "https://somafm.com/support/"; }

std::string SomaFMService::NormalizeQuality(const std::string &quality) {
  const std::string lower = StrUtils::ToLower(quality);
  if (lower == "128" || lower == "256" || lower == "high") {
    return "highest";
  }
  if (lower == "64" || lower == "slow") {
    return "low";
  }
  if (lower.empty()) {
    return kQualityDefault;
  }
  return lower;
}

std::vector<RadioChannel> SomaFMService::ParseChannels(const std::string &json, const std::string &quality) {
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
  const std::string preferred = NormalizeQuality(quality);
  JsonArray *array = json_object_get_array_member(object, "channels");
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *channel_obj = json_node_get_object(item);
    if (!json_object_has_member(channel_obj, "title") || !json_object_has_member(channel_obj, "image")) {
      continue;
    }
    const char *title = json_object_get_string_member(channel_obj, "title");
    const char *image = json_object_get_string_member(channel_obj, "image");
    if (!title || !json_object_has_member(channel_obj, "playlists") ||
        !JSON_NODE_HOLDS_ARRAY(json_object_get_member(channel_obj, "playlists"))) {
      continue;
    }
    JsonArray *playlists = json_object_get_array_member(channel_obj, "playlists");
    const guint playlist_count = json_array_get_length(playlists);
    for (guint p = 0; p < playlist_count; ++p) {
      JsonNode *playlist_node = json_array_get_element(playlists, p);
      if (!playlist_node || !JSON_NODE_HOLDS_OBJECT(playlist_node)) {
        continue;
      }
      JsonObject *playlist = json_node_get_object(playlist_node);
      if (!json_object_has_member(playlist, "url") || !json_object_has_member(playlist, "quality")) {
        continue;
      }
      const char *playlist_quality = json_object_get_string_member(playlist, "quality");
      const char *url = json_object_get_string_member(playlist, "url");
      if (!url || !playlist_quality || NormalizeQuality(playlist_quality) != preferred) {
        continue;
      }
      RadioChannel channel;
      channel.source = Song::Source::SomaFM;
      channel.name = title;
      channel.url = url;
      channel.thumbnail_url = image ? image : "";
      if (json_object_has_member(playlist, "format")) {
        const char *format = json_object_get_string_member(playlist, "format");
        if (format && *format) {
          channel.name += " " + StrUtils::ToUpper(format);
        }
      }
      channels.push_back(channel);
    }
  }
  g_object_unref(parser);
  return channels;
}
