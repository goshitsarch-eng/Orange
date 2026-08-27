#include "scrobbler/scrobblercache.h"

#include "core/standardpaths.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>
#include <cstdlib>

namespace {

std::string ObjectString(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return {};
  }
  JsonNode *node = json_object_get_member(object, name);
  if (json_node_get_value_type(node) == G_TYPE_STRING) {
    const char *value = json_node_get_string(node);
    return value ? value : "";
  }
  if (json_node_get_value_type(node) == G_TYPE_INT64) {
    return std::to_string(json_node_get_int(node));
  }
  return {};
}

int64_t ObjectInt64(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name) || !JSON_NODE_HOLDS_VALUE(json_object_get_member(object, name))) {
    return 0;
  }
  JsonNode *node = json_object_get_member(object, name);
  if (json_node_get_value_type(node) == G_TYPE_INT64) {
    return json_node_get_int(node);
  }
  return std::strtoll(ObjectString(object, name).c_str(), nullptr, 10);
}

}  // namespace

ScrobblerCache::ScrobblerCache(const std::string &filename) : path_(FileUtils::Join(StandardPaths::CacheDir(), filename)) { Load(); }

void ScrobblerCache::Add(const Song &song, uint64_t timestamp) {
  if (song.artist().empty() || song.title().empty() || timestamp == 0) {
    return;
  }
  ScrobblerCacheItem item;
  item.timestamp = timestamp;
  item.artist = song.artist();
  item.album = song.album();
  item.title = song.title();
  item.albumartist = song.albumartist();
  item.track = song.track();
  item.length_nanosec = song.length_nanosec();
  items_.push_back(item);
  Save();
}

std::vector<ScrobblerCacheItem> ScrobblerCache::Unsent() const {
  std::vector<ScrobblerCacheItem> unsent;
  for (const ScrobblerCacheItem &item : items_) {
    if (!item.sent) {
      unsent.push_back(item);
    }
  }
  return unsent;
}

void ScrobblerCache::MarkSent() {
  for (ScrobblerCacheItem &item : items_) {
    item.sent = true;
  }
}

void ScrobblerCache::ClearSent() {
  for (ScrobblerCacheItem &item : items_) {
    item.sent = false;
  }
}

void ScrobblerCache::RemoveSent() {
  items_.erase(std::remove_if(items_.begin(), items_.end(), [](const ScrobblerCacheItem &item) { return item.sent; }), items_.end());
  Save();
}

void ScrobblerCache::Load() { items_ = Parse(FileUtils::ReadFile(path_)); }

void ScrobblerCache::Save() const {
  if (items_.empty()) {
    FileUtils::Remove(path_);
    return;
  }
  FileUtils::WriteFile(path_, ToJson(items_));
}

std::string ScrobblerCache::ToJson(const std::vector<ScrobblerCacheItem> &items) {
  std::string json = "{\"tracks\":[";
  for (size_t i = 0; i < items.size(); ++i) {
    if (i) {
      json += ",";
    }
    json += "{\"timestamp\":" + std::to_string(items[i].timestamp) + ",\"artist\":\"" + StrUtils::JsonEscape(items[i].artist) + "\",\"album\":\"" +
            StrUtils::JsonEscape(items[i].album) + "\",\"title\":\"" + StrUtils::JsonEscape(items[i].title) + "\",\"track\":" +
            std::to_string(items[i].track) + ",\"albumartist\":\"" + StrUtils::JsonEscape(items[i].albumartist) + "\",\"length_nanosec\":" +
            std::to_string(items[i].length_nanosec) + "}";
  }
  json += "]}";
  return json;
}

std::vector<ScrobblerCacheItem> ScrobblerCache::Parse(const std::string &json) {
  std::vector<ScrobblerCacheItem> items;
  if (json.empty()) {
    return items;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return items;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return items;
  }
  JsonObject *object = json_node_get_object(root);
  if (!json_object_has_member(object, "tracks") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "tracks"))) {
    g_object_unref(parser);
    return items;
  }
  JsonArray *tracks = json_object_get_array_member(object, "tracks");
  const guint n = json_array_get_length(tracks);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item_node = json_array_get_element(tracks, i);
    if (!item_node || !JSON_NODE_HOLDS_OBJECT(item_node)) {
      continue;
    }
    JsonObject *track = json_node_get_object(item_node);
    ScrobblerCacheItem item;
    item.timestamp = static_cast<uint64_t>(ObjectInt64(track, "timestamp"));
    item.artist = ObjectString(track, "artist");
    item.album = ObjectString(track, "album");
    item.title = ObjectString(track, "title");
    item.albumartist = ObjectString(track, "albumartist");
    item.track = static_cast<int>(ObjectInt64(track, "track"));
    item.length_nanosec = ObjectInt64(track, "length_nanosec");
    if (item.timestamp == 0 || item.artist.empty() || item.title.empty()) {
      continue;
    }
    items.push_back(item);
  }
  g_object_unref(parser);
  return items;
}
