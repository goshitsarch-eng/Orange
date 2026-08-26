#include "utilities/jsonutils.h"

#include "utilities/strutils.h"

#include <json-glib/json-glib.h>

#include <algorithm>
#include <cctype>
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

std::string ExtractLyrics(const std::string &body) {
  if (body.empty()) {
    return {};
  }
  if (body.find('{') == 0 || body.find("\"lyrics\"") != std::string::npos || body.find("plainLyrics") != std::string::npos) {
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

}  // namespace JsonUtils
