#include "utilities/jsonutils.h"

#include "streaming/streamingalbum.h"
#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <functional>

namespace {

bool IsHttpUrl(const std::string &value) {
  return StrUtils::StartsWith(value, "http://") || StrUtils::StartsWith(value, "https://");
}

bool LooksLikeImageUrl(const std::string &value) {
  if (!IsHttpUrl(value)) {
    return false;
  }
  const std::string lower = StrUtils::ToLower(value);
  return lower.find(".jpg") != std::string::npos || lower.find(".jpeg") != std::string::npos || lower.find(".png") != std::string::npos ||
         lower.find(".webp") != std::string::npos || lower.find(".gif") != std::string::npos || lower.find("cover") != std::string::npos ||
         lower.find("image") != std::string::npos || lower.find("artwork") != std::string::npos || lower.find("albumart") != std::string::npos ||
         lower.find("coverartarchive") != std::string::npos;
}

int ImageUrlScore(const std::string &key, const std::string &value) {
  int score = 1;
  const std::string lower_key = StrUtils::ToLower(key);
  const std::string lower_value = StrUtils::ToLower(value);
  if (lower_key.find("xl") != std::string::npos || lower_key.find("extra") != std::string::npos || lower_key.find("mega") != std::string::npos) {
    score += 50;
  } else if (lower_key.find("large") != std::string::npos || lower_key.find("big") != std::string::npos || lower_key == "#text") {
    score += 30;
  } else if (lower_key.find("medium") != std::string::npos) {
    score += 15;
  }
  if (lower_value.find("1200") != std::string::npos || lower_value.find("1000") != std::string::npos) {
    score += 20;
  }
  if (LooksLikeImageUrl(value)) {
    score += 10;
  }
  return score;
}

void WalkNode(JsonNode *node, const std::string &key, const std::function<void(const std::string &, JsonNode *)> &visit) {
  if (!node) {
    return;
  }
  visit(key, node);
  if (JSON_NODE_HOLDS_OBJECT(node)) {
    JsonObject *object = json_node_get_object(node);
    GList *members = json_object_get_members(object);
    for (GList *it = members; it; it = it->next) {
      const char *member = static_cast<const char *>(it->data);
      WalkNode(json_object_get_member(object, member), member ? member : "", visit);
    }
    g_list_free(members);
  } else if (JSON_NODE_HOLDS_ARRAY(node)) {
    JsonArray *array = json_node_get_array(node);
    const guint n = json_array_get_length(array);
    for (guint i = 0; i < n; ++i) {
      WalkNode(json_array_get_element(array, i), key, visit);
    }
  }
}

JsonNode *Parse(const std::string &json) {
  if (json.empty()) {
    return nullptr;
  }
  JsonParser *parser = json_parser_new();
  GError *error = nullptr;
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), &error)) {
    if (error) {
      g_error_free(error);
    }
    g_object_unref(parser);
    return nullptr;
  }
  JsonNode *root = json_parser_steal_root(parser);
  g_object_unref(parser);
  return root;
}

std::string NodeString(JsonNode *node) {
  if (!node) {
    return {};
  }
  if (JSON_NODE_HOLDS_VALUE(node)) {
    const GType type = json_node_get_value_type(node);
    if (type == G_TYPE_STRING) {
      const char *value = json_node_get_string(node);
      return value ? value : "";
    }
    if (type == G_TYPE_INT64) {
      return std::to_string(json_node_get_int(node));
    }
    if (type == G_TYPE_DOUBLE) {
      return std::to_string(json_node_get_double(node));
    }
  }
  return {};
}

}  // namespace

namespace JsonUtils {

bool LooksLikeImage(const std::string &data) {
  if (data.size() < 8) {
    return false;
  }
  const auto *bytes = reinterpret_cast<const unsigned char *>(data.data());
  if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
    return true;
  }
  if (bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47) {
    return true;
  }
  if (bytes[0] == 0x47 && bytes[1] == 0x49 && bytes[2] == 0x46) {
    return true;
  }
  if (data.compare(0, 4, "RIFF") == 0 && data.size() > 12 && data.compare(8, 4, "WEBP") == 0) {
    return true;
  }
  return false;
}

std::string StripHtml(const std::string &html) {
  std::string text;
  text.reserve(html.size());
  bool in_tag = false;
  bool skip = false;
  std::string tag;
  for (size_t i = 0; i < html.size(); ++i) {
    const char ch = html[i];
    if (ch == '<') {
      in_tag = true;
      tag.clear();
      continue;
    }
    if (in_tag) {
      if (ch == '>') {
        const std::string lower = StrUtils::ToLower(tag);
        if (StrUtils::StartsWith(lower, "script") || StrUtils::StartsWith(lower, "style")) {
          skip = true;
        }
        if (StrUtils::StartsWith(lower, "/script") || StrUtils::StartsWith(lower, "/style")) {
          skip = false;
        }
        if (StrUtils::StartsWith(lower, "br") || StrUtils::StartsWith(lower, "p") || StrUtils::StartsWith(lower, "/p") ||
            StrUtils::StartsWith(lower, "div") || StrUtils::StartsWith(lower, "/div") || StrUtils::StartsWith(lower, "li")) {
          text.push_back('\n');
        }
        in_tag = false;
      } else if (tag.size() < 32) {
        tag.push_back(ch);
      }
      continue;
    }
    if (!skip) {
      text.push_back(ch);
    }
  }
  text = StrUtils::Replace(text, "&nbsp;", " ");
  text = StrUtils::Replace(text, "&amp;", "&");
  text = StrUtils::Replace(text, "&lt;", "<");
  text = StrUtils::Replace(text, "&gt;", ">");
  text = StrUtils::Replace(text, "&quot;", "\"");
  text = StrUtils::Replace(text, "&#39;", "'");
  std::string collapsed;
  collapsed.reserve(text.size());
  int newlines = 0;
  for (char ch : text) {
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      if (newlines < 2) {
        collapsed.push_back('\n');
      }
      ++newlines;
      continue;
    }
    newlines = 0;
    collapsed.push_back(ch);
  }
  return StrUtils::Trim(collapsed);
}

std::string GetString(const std::string &json, const std::vector<std::string> &path) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonNode *node = root;
  for (const std::string &part : path) {
    if (!node || !JSON_NODE_HOLDS_OBJECT(node)) {
      json_node_unref(root);
      return {};
    }
    node = json_object_get_member(json_node_get_object(node), part.c_str());
  }
  const std::string value = NodeString(node);
  json_node_unref(root);
  return value;
}

int GetInt(const std::string &json, const std::vector<std::string> &path, int fallback) {
  const std::string value = GetString(json, path);
  if (value.empty()) {
    return fallback;
  }
  return static_cast<int>(std::strtol(value.c_str(), nullptr, 10));
}

double GetDouble(const std::string &json, const std::vector<std::string> &path, double fallback) {
  const std::string value = GetString(json, path);
  if (value.empty()) {
    return fallback;
  }
  return std::strtod(value.c_str(), nullptr);
}

std::string FindStringByKeys(const std::string &json, const std::vector<std::string> &keys) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  std::string found;
  WalkNode(root, {}, [&](const std::string &key, JsonNode *node) {
    if (!found.empty()) {
      return;
    }
    for (const std::string &wanted : keys) {
      if (StrUtils::ToLower(key) == StrUtils::ToLower(wanted)) {
        const std::string value = NodeString(node);
        if (!value.empty()) {
          found = value;
          return;
        }
      }
    }
  });
  json_node_unref(root);
  return found;
}

std::string FindFirstImageUrl(const std::string &json) {
  return FindCoverUrl(json);
}

std::string FindCoverUrl(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  std::string best;
  int best_score = 0;
  std::string release_id;
  WalkNode(root, {}, [&](const std::string &key, JsonNode *node) {
    if (node && JSON_NODE_HOLDS_OBJECT(node)) {
      JsonObject *object = json_node_get_object(node);
      if (json_object_has_member(object, "#text") && json_object_has_member(object, "size")) {
        const std::string url = NodeString(json_object_get_member(object, "#text"));
        const std::string size = NodeString(json_object_get_member(object, "size"));
        if (IsHttpUrl(url)) {
          const int score = ImageUrlScore(size, url) + 5;
          if (score > best_score) {
            best_score = score;
            best = url;
          }
        }
      }
    }
    const std::string value = NodeString(node);
    if (value.empty()) {
      return;
    }
    const std::string lower_key = StrUtils::ToLower(key);
    if (release_id.empty() && (lower_key == "id" || lower_key == "mbid") && value.size() == 36 && value.find('-') != std::string::npos) {
      release_id = value;
    }
    if (!IsHttpUrl(value)) {
      return;
    }
    if (LooksLikeImageUrl(value) || lower_key == "#text" || lower_key.find("cover") != std::string::npos ||
        lower_key.find("image") != std::string::npos || lower_key.find("thumb") != std::string::npos ||
        lower_key.find("picture") != std::string::npos) {
      const int score = ImageUrlScore(key, value);
      if (score > best_score) {
        best_score = score;
        best = value;
      }
    }
  });
  json_node_unref(root);
  if (best.empty() && !release_id.empty()) {
    return "https://coverartarchive.org/release/" + release_id + "/front";
  }
  return best;
}

std::vector<std::string> FindAllCoverUrls(const std::string &json) {
  std::vector<std::string> urls;
  JsonNode *root = Parse(json);
  if (!root) {
    return urls;
  }
  struct Hit {
    std::string url;
    int score = 0;
  };
  std::vector<Hit> hits;
  std::string release_id;
  WalkNode(root, {}, [&](const std::string &key, JsonNode *node) {
    if (node && JSON_NODE_HOLDS_OBJECT(node)) {
      JsonObject *object = json_node_get_object(node);
      if (json_object_has_member(object, "#text") && json_object_has_member(object, "size")) {
        const std::string url = NodeString(json_object_get_member(object, "#text"));
        const std::string size = NodeString(json_object_get_member(object, "size"));
        if (IsHttpUrl(url)) {
          hits.push_back({url, ImageUrlScore(size, url) + 5});
        }
      }
    }
    const std::string value = NodeString(node);
    if (value.empty()) {
      return;
    }
    const std::string lower_key = StrUtils::ToLower(key);
    if (release_id.empty() && (lower_key == "id" || lower_key == "mbid") && value.size() == 36 && value.find('-') != std::string::npos) {
      release_id = value;
    }
    if (!IsHttpUrl(value)) {
      return;
    }
    if (LooksLikeImageUrl(value) || lower_key == "#text" || lower_key.find("cover") != std::string::npos ||
        lower_key.find("image") != std::string::npos || lower_key.find("thumb") != std::string::npos ||
        lower_key.find("picture") != std::string::npos) {
      hits.push_back({value, ImageUrlScore(key, value)});
    }
  });
  json_node_unref(root);
  std::sort(hits.begin(), hits.end(), [](const Hit &a, const Hit &b) { return a.score > b.score; });
  for (const Hit &hit : hits) {
    if (std::find(urls.begin(), urls.end(), hit.url) != urls.end()) {
      continue;
    }
    urls.push_back(hit.url);
  }
  if (urls.empty() && !release_id.empty()) {
    urls.push_back("https://coverartarchive.org/release/" + release_id + "/front");
  }
  return urls;
}

std::string ExtractLyrics(const std::string &body) {
  if (body.empty()) {
    return {};
  }
  if (body.find('{') == 0 || body.find("\"lyrics\"") != std::string::npos || body.find("plainLyrics") != std::string::npos) {
    const std::string synced = FindStringByKeys(body, {"syncedLyrics"});
    if (!synced.empty()) {
      return StrUtils::Trim(synced);
    }
    const std::string lyrics = FindStringByKeys(body, {"plainLyrics", "lyrics", "lyric", "unsyncedLyrics", "text"});
    if (!lyrics.empty()) {
      return StrUtils::Trim(lyrics);
    }
  }
  if (body.find('<') != std::string::npos) {
    std::string text = StripHtml(body);
    if (text.size() > 80) {
      return text;
    }
  }
  if (body.find('{') == std::string::npos && body.size() > 40) {
    return StrUtils::Trim(body);
  }
  return {};
}

SongList ParseSongs(const std::string &json, Song::Source source) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  SongList songs;
  auto add_object = [&](JsonObject *object) {
    if (!object) {
      return;
    }
    auto member = [&](const char *name) -> std::string {
      if (!json_object_has_member(object, name)) {
        return {};
      }
      return NodeString(json_object_get_member(object, name));
    };
    Song song(source);
    std::string title = member("title");
    if (title.empty()) {
      title = member("name");
    }
    if (title.empty()) {
      title = member("track");
    }
    std::string artist = member("artist");
    if (artist.empty() && json_object_has_member(object, "artist") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "artist"))) {
      artist = NodeString(json_object_get_member(json_object_get_object_member(object, "artist"), "name"));
    }
    if (artist.empty()) {
      artist = member("artistName");
    }
    std::string album = member("album");
    if (album.empty() && json_object_has_member(object, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "album"))) {
      album = NodeString(json_object_get_member(json_object_get_object_member(object, "album"), "title"));
      if (album.empty()) {
        album = NodeString(json_object_get_member(json_object_get_object_member(object, "album"), "name"));
      }
    }
    std::string url = member("url");
    if (url.empty()) {
      url = member("uri");
    }
    if (url.empty()) {
      url = member("stream_url");
    }
    if (url.empty()) {
      url = member("preview");
    }
    if (title.empty() && url.empty()) {
      return;
    }
    song.set_title(title);
    song.set_artist(artist);
    song.set_album(album);
    song.set_url(url);
    song.set_valid(true);
    songs.push_back(song);
  };

  WalkNode(root, {}, [&](const std::string &, JsonNode *node) {
    if (node && JSON_NODE_HOLDS_OBJECT(node)) {
      JsonObject *object = json_node_get_object(node);
      if (json_object_has_member(object, "title") || json_object_has_member(object, "name") || json_object_has_member(object, "url")) {
        if (json_object_has_member(object, "title") || json_object_has_member(object, "preview") || json_object_has_member(object, "duration") ||
            json_object_has_member(object, "stream_url")) {
          add_object(object);
        }
      }
    }
  });
  json_node_unref(root);
  return songs;
}

namespace {

Song SongFromMusicBrainzRecording(JsonObject *recording) {
  Song song(Song::Source::LocalFile);
  if (!recording) {
    return song;
  }
  song.set_title(NodeString(json_object_get_member(recording, "title")));
  song.set_musicbrainz_recording_id(NodeString(json_object_get_member(recording, "id")));
  if (json_object_has_member(recording, "length") && JSON_NODE_HOLDS_VALUE(json_object_get_member(recording, "length"))) {
    const gint64 length_msec = json_node_get_int(json_object_get_member(recording, "length"));
    if (length_msec > 0) {
      song.set_length_nanosec(length_msec * 1000000);
    }
  }
  if (json_object_has_member(recording, "artist-credit") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(recording, "artist-credit"))) {
    JsonArray *credits = json_object_get_array_member(recording, "artist-credit");
    if (json_array_get_length(credits) > 0) {
      JsonObject *credit = json_array_get_object_element(credits, 0);
      std::string artist = NodeString(json_object_get_member(credit, "name"));
      if (json_object_has_member(credit, "artist") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(credit, "artist"))) {
        JsonObject *artist_object = json_object_get_object_member(credit, "artist");
        if (artist.empty()) {
          artist = NodeString(json_object_get_member(artist_object, "name"));
        }
        song.set_musicbrainz_artist_id(NodeString(json_object_get_member(artist_object, "id")));
      }
      song.set_artist(artist);
    }
  }
  if (json_object_has_member(recording, "releases") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(recording, "releases"))) {
    JsonArray *releases = json_object_get_array_member(recording, "releases");
    if (json_array_get_length(releases) > 0) {
      JsonObject *release = json_array_get_object_element(releases, 0);
      song.set_album(NodeString(json_object_get_member(release, "title")));
      song.set_musicbrainz_album_id(NodeString(json_object_get_member(release, "id")));
      if (json_object_has_member(release, "artist-credit") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(release, "artist-credit"))) {
        JsonArray *album_credits = json_object_get_array_member(release, "artist-credit");
        if (json_array_get_length(album_credits) > 0) {
          JsonObject *album_credit = json_array_get_object_element(album_credits, 0);
          std::string album_artist = NodeString(json_object_get_member(album_credit, "name"));
          if (json_object_has_member(album_credit, "artist") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(album_credit, "artist"))) {
            JsonObject *album_artist_object = json_object_get_object_member(album_credit, "artist");
            if (album_artist.empty()) {
              album_artist = NodeString(json_object_get_member(album_artist_object, "name"));
            }
            song.set_musicbrainz_album_artist_id(NodeString(json_object_get_member(album_artist_object, "id")));
          }
          song.set_albumartist(album_artist);
        }
      }
      const std::string date = NodeString(json_object_get_member(release, "date"));
      if (date.size() >= 4) {
        song.set_year(std::atoi(date.c_str()));
      }
    }
  }
  return song;
}

void AppendMusicBrainzRecording(SongList *songs, JsonObject *recording) {
  if (!songs || !recording) {
    return;
  }
  Song song = SongFromMusicBrainzRecording(recording);
  if (song.title().empty()) {
    return;
  }
  song.set_valid(true);
  songs->push_back(song);
}

}  // namespace

SongList ParseMusicBrainzRecordings(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
    if (root) {
      json_node_unref(root);
    }
    return {};
  }
  JsonObject *object = json_node_get_object(root);
  SongList songs;
  if (json_object_has_member(object, "recordings") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "recordings"))) {
    JsonArray *recordings = json_object_get_array_member(object, "recordings");
    const guint n = json_array_get_length(recordings);
    for (guint i = 0; i < n; ++i) {
      JsonNode *item = json_array_get_element(recordings, i);
      if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
        continue;
      }
      AppendMusicBrainzRecording(&songs, json_node_get_object(item));
    }
  } else if (json_object_has_member(object, "title") && json_object_has_member(object, "id")) {
    AppendMusicBrainzRecording(&songs, object);
  }
  json_node_unref(root);
  return songs;
}

namespace {

std::string ObjectString(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name)) {
    return {};
  }
  return NodeString(json_object_get_member(object, name));
}

bool ObjectBool(JsonObject *object, const char *name) {
  if (!object || !json_object_has_member(object, name)) {
    return false;
  }
  JsonNode *node = json_object_get_member(object, name);
  if (!node || !JSON_NODE_HOLDS_VALUE(node)) {
    return false;
  }
  if (json_node_get_value_type(node) == G_TYPE_BOOLEAN) {
    return json_node_get_boolean(node) != FALSE;
  }
  const std::string text = NodeString(node);
  return text == "true" || text == "1";
}

std::string NestedName(JsonObject *object, const char *member, const char *name_key) {
  if (!object || !json_object_has_member(object, member) || !JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, member))) {
    return {};
  }
  return ObjectString(json_object_get_object_member(object, member), name_key);
}

void ApplyDurationSeconds(Song *song, const std::string &seconds) {
  if (seconds.empty()) {
    return;
  }
  song->set_length_nanosec(static_cast<int64_t>(std::strtod(seconds.c_str(), nullptr) * 1000000000.0));
}

SongList SongsFromArray(JsonArray *array, Song::Source source, const std::function<void(Song *, JsonObject *)> &fill) {
  SongList songs;
  if (!array) {
    return songs;
  }
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    Song song(source);
    JsonObject *object = json_node_get_object(item);
    if (json_object_has_member(object, "item") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "item"))) {
      object = json_object_get_object_member(object, "item");
    } else if (json_object_has_member(object, "track") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "track"))) {
      object = json_object_get_object_member(object, "track");
    } else if (!json_object_has_member(object, "name") && !json_object_has_member(object, "title") && json_object_has_member(object, "album") &&
               JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "album"))) {
      object = json_object_get_object_member(object, "album");
    }
    fill(&song, object);
    if (song.title().empty()) {
      song.set_title(ObjectString(object, "name"));
    }
    if (!song.title().empty()) {
      song.set_valid(true);
      songs.push_back(song);
    }
  }
  return songs;
}

JsonArray *FindNamedArray(JsonNode *root, const std::vector<std::string> &path) {
  JsonNode *node = root;
  for (const std::string &part : path) {
    if (!node || !JSON_NODE_HOLDS_OBJECT(node)) {
      return nullptr;
    }
    node = json_object_get_member(json_node_get_object(node), part.c_str());
  }
  if (node && JSON_NODE_HOLDS_ARRAY(node)) {
    return json_node_get_array(node);
  }
  return nullptr;
}

JsonObject *FindNamedObject(JsonNode *root, const std::vector<std::string> &path) {
  JsonNode *node = root;
  for (const std::string &part : path) {
    if (!node || !JSON_NODE_HOLDS_OBJECT(node)) {
      return nullptr;
    }
    node = json_object_get_member(json_node_get_object(node), part.c_str());
  }
  if (node && JSON_NODE_HOLDS_OBJECT(node)) {
    return json_node_get_object(node);
  }
  return nullptr;
}

int QobuzAlbumYear(JsonObject *object) {
  const int year = std::atoi(ObjectString(object, "year").c_str());
  if (year > 0) {
    return year;
  }
  const std::string date = ObjectString(object, "release_date_original");
  if (date.size() >= 4) {
    const int from_date = std::atoi(date.c_str());
    if (from_date > 0) {
      return from_date;
    }
  }
  const std::string released = ObjectString(object, "released_at");
  if (released.empty()) {
    return 0;
  }
  const time_t timestamp = static_cast<time_t>(std::strtoll(released.c_str(), nullptr, 10));
  if (timestamp <= 0) {
    return 0;
  }
  struct tm tm {};
  if (!gmtime_r(&timestamp, &tm)) {
    return 0;
  }
  return tm.tm_year + 1900;
}

}  // namespace

SongList ParseSubsonicSongs(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *songs_array = FindNamedArray(root, {"subsonic-response", "searchResult3", "song"});
  if (!songs_array) {
    songs_array = FindNamedArray(root, {"subsonic-response", "starred2", "song"});
  }
  if (!songs_array) {
    songs_array = FindNamedArray(root, {"subsonic-response", "starred", "song"});
  }
  if (!songs_array) {
    songs_array = FindNamedArray(root, {"subsonic-response", "album", "song"});
  }
  SongList songs = SongsFromArray(songs_array, Song::Source::Subsonic, [](Song *song, JsonObject *object) {
    song->set_title(ObjectString(object, "title"));
    song->set_artist(ObjectString(object, "artist"));
    song->set_album(ObjectString(object, "album"));
    song->set_albumartist(ObjectString(object, "albumArtist"));
    song->set_genre(ObjectString(object, "genre"));
    const std::string id = ObjectString(object, "id");
    song->set_song_id(id);
    song->set_artist_id(ObjectString(object, "artistId"));
    song->set_album_id(ObjectString(object, "albumId"));
    song->set_url(id.empty() ? std::string() : "subsonic://" + id);
    song->set_year(std::atoi(ObjectString(object, "year").c_str()));
    song->set_track(std::atoi(ObjectString(object, "track").c_str()));
    song->set_disc(std::atoi(ObjectString(object, "discNumber").c_str()));
    song->set_bitrate(std::atoi(ObjectString(object, "bitRate").c_str()));
    song->set_filesize(std::strtoll(ObjectString(object, "size").c_str(), nullptr, 10));
    ApplyDurationSeconds(song, ObjectString(object, "duration"));
    const std::string cover = ObjectString(object, "coverArt");
    if (!cover.empty()) {
      song->set_art_automatic(cover);
    }
  });
  JsonObject *album_object = FindNamedObject(root, {"subsonic-response", "album"});
  if (album_object && json_object_has_member(album_object, "song")) {
    Song album(Song::Source::Subsonic);
    album.set_album(ObjectString(album_object, "name"));
    if (album.album().empty()) {
      album.set_album(ObjectString(album_object, "album"));
    }
    album.set_album_id(ObjectString(album_object, "id"));
    album.set_artist(ObjectString(album_object, "artist"));
    album.set_albumartist(ObjectString(album_object, "albumArtist"));
    if (album.albumartist().empty()) {
      album.set_albumartist(album.artist());
    }
    album.set_artist_id(ObjectString(album_object, "artistId"));
    album.set_genre(ObjectString(album_object, "genre"));
    album.set_year(std::atoi(ObjectString(album_object, "year").c_str()));
    const std::string cover = ObjectString(album_object, "coverArt");
    if (!cover.empty()) {
      album.set_art_automatic(cover);
    }
    StreamingAlbum::ApplyParent(songs, album);
  }
  json_node_unref(root);
  return songs;
}

SongList ParseTidalTracks(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"items"});
  if (!items) {
    items = FindNamedArray(root, {"tracks", "items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Tidal, [](Song *song, JsonObject *object) {
    song->set_title(ObjectString(object, "title"));
    song->set_artist(NestedName(object, "artist", "name"));
    song->set_album(NestedName(object, "album", "title"));
    if (song->album().empty()) {
      song->set_album(NestedName(object, "album", "name"));
    }
    const std::string id = ObjectString(object, "id");
    song->set_song_id(id);
    song->set_artist_id(NestedName(object, "artist", "id"));
    song->set_album_id(NestedName(object, "album", "id"));
    song->set_url(id.empty() ? std::string() : "tidal://" + id);
    ApplyDurationSeconds(song, ObjectString(object, "duration"));
    const std::string cover = NestedName(object, "album", "cover");
    if (!cover.empty()) {
      song->set_art_automatic("https://resources.tidal.com/images/" + StrUtils::Replace(cover, "-", "/") + "/1280x1280.jpg");
    }
    if (ObjectBool(object, "explicit")) {
      song->set_comment("explicit");
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseSpotifyTracks(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"tracks", "items"});
  if (!items) {
    items = FindNamedArray(root, {"artists", "items"});
  }
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Spotify, [](Song *song, JsonObject *object) {
    song->set_title(ObjectString(object, "name"));
    if (json_object_has_member(object, "artists") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "artists"))) {
      JsonArray *artists = json_object_get_array_member(object, "artists");
      if (json_array_get_length(artists) > 0) {
        song->set_artist(ObjectString(json_array_get_object_element(artists, 0), "name"));
        song->set_artist_id(ObjectString(json_array_get_object_element(artists, 0), "id"));
      }
    }
    song->set_album(NestedName(object, "album", "name"));
    song->set_album_id(NestedName(object, "album", "id"));
    const std::string id = ObjectString(object, "id");
    song->set_song_id(id);
    song->set_url(id.empty() ? std::string() : "spotify://" + id);
    const std::string preview = ObjectString(object, "preview_url");
    if (!preview.empty()) {
      song->set_stream_url(preview);
    }
    const std::string ms = ObjectString(object, "duration_ms");
    if (!ms.empty()) {
      song->set_length_nanosec(std::strtoll(ms.c_str(), nullptr, 10) * 1000000LL);
    }
    if (json_object_has_member(object, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "album"))) {
      JsonObject *album = json_object_get_object_member(object, "album");
      if (json_object_has_member(album, "images") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(album, "images"))) {
        JsonArray *images = json_object_get_array_member(album, "images");
        if (json_array_get_length(images) > 0) {
          song->set_art_automatic(ObjectString(json_array_get_object_element(images, 0), "url"));
        }
      }
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseQobuzTracks(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"tracks", "items"});
  if (!items) {
    items = FindNamedArray(root, {"albums", "items"});
  }
  if (!items) {
    items = FindNamedArray(root, {"artists", "items"});
  }
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Qobuz, [](Song *song, JsonObject *object) {
    song->set_title(ObjectString(object, "title"));
    song->set_artist(NestedName(object, "performer", "name"));
    if (song->artist().empty()) {
      song->set_artist(NestedName(object, "artist", "name"));
    }
    song->set_album(NestedName(object, "album", "title"));
    const std::string id = ObjectString(object, "id");
    song->set_song_id(id);
    song->set_artist_id(NestedName(object, "performer", "id"));
    if (song->artist_id().empty()) {
      song->set_artist_id(NestedName(object, "artist", "id"));
    }
    song->set_album_id(NestedName(object, "album", "id"));
    song->set_url(id.empty() ? std::string() : "qobuz://" + id);
    ApplyDurationSeconds(song, ObjectString(object, "duration"));
    const std::string image = NestedName(object, "album", "image");
    if (!image.empty()) {
      song->set_art_automatic(image);
    } else if (json_object_has_member(object, "album") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "album"))) {
      JsonObject *album = json_object_get_object_member(object, "album");
      if (json_object_has_member(album, "image") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(album, "image"))) {
        song->set_art_automatic(ObjectString(json_object_get_object_member(album, "image"), "large"));
      }
    }
  });
  // album/get puts album title/artist/art on the root object, not on each track.
  if (root && JSON_NODE_HOLDS_OBJECT(root) && FindNamedArray(root, {"tracks", "items"})) {
    JsonObject *object = json_node_get_object(root);
    const std::string title = ObjectString(object, "title");
    if (!title.empty()) {
      Song album(Song::Source::Qobuz);
      album.set_album(title);
      album.set_album_id(ObjectString(object, "id"));
      album.set_artist(NestedName(object, "artist", "name"));
      album.set_albumartist(album.artist());
      album.set_artist_id(NestedName(object, "artist", "id"));
      album.set_genre(NestedName(object, "genre", "name"));
      album.set_year(QobuzAlbumYear(object));
      const std::string image = NestedName(object, "image", "large");
      if (!image.empty()) {
        album.set_art_automatic(image);
      }
      StreamingAlbum::ApplyParent(songs, album);
    }
  }
  json_node_unref(root);
  return songs;
}

SongList ParseTidalArtists(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"items"});
  if (!items) {
    items = FindNamedArray(root, {"artists", "items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Tidal, [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_artist(name);
    const std::string id = ObjectString(object, "id");
    song->set_artist_id(id);
    song->set_url(id.empty() ? std::string() : "tidal://artist/" + id);
  });
  json_node_unref(root);
  return songs;
}

SongList ParseTidalAlbums(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"items"});
  if (!items) {
    items = FindNamedArray(root, {"albums", "items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Tidal, [](Song *song, JsonObject *object) {
    std::string title = ObjectString(object, "title");
    if (title.empty()) {
      title = ObjectString(object, "name");
    }
    song->set_title(title);
    song->set_album(title);
    song->set_artist(NestedName(object, "artist", "name"));
    const std::string id = ObjectString(object, "id");
    song->set_album_id(id);
    song->set_artist_id(NestedName(object, "artist", "id"));
    song->set_url(id.empty() ? std::string() : "tidal://album/" + id);
    const std::string cover = ObjectString(object, "cover");
    if (!cover.empty()) {
      song->set_art_automatic("https://resources.tidal.com/images/" + StrUtils::Replace(cover, "-", "/") + "/1280x1280.jpg");
    }
    if (ObjectBool(object, "explicit")) {
      song->set_comment("explicit");
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseSpotifyArtists(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"artists", "items"});
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Spotify, [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_artist(name);
    const std::string id = ObjectString(object, "id");
    song->set_artist_id(id);
    song->set_url(id.empty() ? std::string() : "spotify://artist/" + id);
    if (json_object_has_member(object, "images") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "images"))) {
      JsonArray *images = json_object_get_array_member(object, "images");
      if (json_array_get_length(images) > 0) {
        song->set_art_automatic(ObjectString(json_array_get_object_element(images, 0), "url"));
      }
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseSpotifyAlbums(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"albums", "items"});
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Spotify, [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_album(name);
    const std::string id = ObjectString(object, "id");
    song->set_album_id(id);
    if (json_object_has_member(object, "artists") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "artists"))) {
      JsonArray *artists = json_object_get_array_member(object, "artists");
      if (json_array_get_length(artists) > 0) {
        song->set_artist(ObjectString(json_array_get_object_element(artists, 0), "name"));
        song->set_artist_id(ObjectString(json_array_get_object_element(artists, 0), "id"));
      }
    }
    song->set_url(id.empty() ? std::string() : "spotify://album/" + id);
    if (json_object_has_member(object, "images") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "images"))) {
      JsonArray *images = json_object_get_array_member(object, "images");
      if (json_array_get_length(images) > 0) {
        song->set_art_automatic(ObjectString(json_array_get_object_element(images, 0), "url"));
      }
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseQobuzArtists(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"artists", "items"});
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Qobuz, [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_artist(name);
    const std::string id = ObjectString(object, "id");
    song->set_artist_id(id);
    song->set_url(id.empty() ? std::string() : "qobuz://artist/" + id);
  });
  json_node_unref(root);
  return songs;
}

SongList ParseQobuzAlbums(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"albums", "items"});
  if (!items) {
    items = FindNamedArray(root, {"items"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Qobuz, [](Song *song, JsonObject *object) {
    const std::string title = ObjectString(object, "title");
    song->set_title(title);
    song->set_album(title);
    song->set_artist(NestedName(object, "artist", "name"));
    const std::string id = ObjectString(object, "id");
    song->set_album_id(id);
    song->set_artist_id(NestedName(object, "artist", "id"));
    song->set_url(id.empty() ? std::string() : "qobuz://album/" + id);
    if (json_object_has_member(object, "image") && JSON_NODE_HOLDS_OBJECT(json_object_get_member(object, "image"))) {
      song->set_art_automatic(ObjectString(json_object_get_object_member(object, "image"), "large"));
    }
  });
  json_node_unref(root);
  return songs;
}

SongList ParseSubsonicArtists(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"subsonic-response", "searchResult3", "artist"});
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "starred2", "artist"});
  }
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "starred", "artist"});
  }
  SongList songs;
  auto fill = [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_artist(name);
    const std::string id = ObjectString(object, "id");
    song->set_artist_id(id);
    song->set_url(id.empty() ? std::string() : "subsonic://artist/" + id);
  };
  if (items) {
    songs = SongsFromArray(items, Song::Source::Subsonic, fill);
  } else if (JsonArray *indexes = FindNamedArray(root, {"subsonic-response", "artists", "index"})) {
    const guint n = json_array_get_length(indexes);
    for (guint i = 0; i < n; ++i) {
      JsonObject *index = json_array_get_object_element(indexes, i);
      if (!index || !json_object_has_member(index, "artist") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(index, "artist"))) {
        continue;
      }
      const SongList page = SongsFromArray(json_object_get_array_member(index, "artist"), Song::Source::Subsonic, fill);
      songs.insert(songs.end(), page.begin(), page.end());
    }
  }
  json_node_unref(root);
  return songs;
}

SongList ParseSubsonicAlbums(const std::string &json) {
  JsonNode *root = Parse(json);
  if (!root) {
    return {};
  }
  JsonArray *items = FindNamedArray(root, {"subsonic-response", "searchResult3", "album"});
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "starred2", "album"});
  }
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "starred", "album"});
  }
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "albumList2", "album"});
  }
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "albumList", "album"});
  }
  if (!items) {
    items = FindNamedArray(root, {"subsonic-response", "artist", "album"});
  }
  SongList songs = SongsFromArray(items, Song::Source::Subsonic, [](Song *song, JsonObject *object) {
    const std::string name = ObjectString(object, "name");
    song->set_title(name);
    song->set_album(name);
    song->set_artist(ObjectString(object, "artist"));
    const std::string id = ObjectString(object, "id");
    song->set_album_id(id);
    song->set_artist_id(ObjectString(object, "artistId"));
    song->set_url(id.empty() ? std::string() : "subsonic://album/" + id);
    song->set_year(std::atoi(ObjectString(object, "year").c_str()));
    const std::string cover = ObjectString(object, "coverArt");
    if (!cover.empty()) {
      song->set_art_automatic(cover);
    }
  });
  json_node_unref(root);
  return songs;
}

}  // namespace JsonUtils
