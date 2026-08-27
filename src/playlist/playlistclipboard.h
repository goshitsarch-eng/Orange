#ifndef STRAWBERRY_PLAYLISTCLIPBOARD_H
#define STRAWBERRY_PLAYLISTCLIPBOARD_H

#include "core/song.h"

#include <string>
#include <vector>

namespace PlaylistClipboard {

// Qt PlaylistItem::EffectiveUrl: stream URL when the item has one, otherwise the media URL.
inline std::string EffectiveUrl(const Song &song) { return song.stream_url(); }

inline std::vector<std::string> EffectiveUrls(const SongList &songs) {
  std::vector<std::string> urls;
  for (const Song &song : songs) {
    const std::string url = EffectiveUrl(song);
    if (!url.empty()) {
      urls.push_back(url);
    }
  }
  return urls;
}

// Qt QMimeData::setUrls + clipboard text(): one URL per line.
inline std::string UrlsText(const std::vector<std::string> &urls) {
  std::string out;
  for (const std::string &url : urls) {
    if (url.empty()) {
      continue;
    }
    if (!out.empty()) {
      out += "\n";
    }
    out += url;
  }
  return out;
}

// Qt CopyCurrentSongToClipboard uses Playlist::Column::URL (song.url()), not the stream URL.
inline std::string ClipboardUrl(const Song &song) { return song.url(); }

// text/uri-list is one URL per line with CRLF (RFC 2483).
inline std::string UriList(const std::vector<std::string> &urls) {
  std::string out;
  for (const std::string &url : urls) {
    if (url.empty()) {
      continue;
    }
    out += url;
    out += "\r\n";
  }
  return out;
}

inline std::string DisplayText(const std::vector<std::string> &column_texts) {
  std::string out;
  for (const std::string &part : column_texts) {
    if (part.empty()) {
      continue;
    }
    if (!out.empty()) {
      out += " - ";
    }
    out += part;
  }
  return out;
}

struct CopyPayload {
  std::string display_text;
  std::vector<std::string> urls;
};

inline CopyPayload FromSong(const Song &song, const std::vector<std::string> &column_texts) {
  CopyPayload payload;
  payload.display_text = DisplayText(column_texts);
  const std::string url = ClipboardUrl(song);
  if (!url.empty()) {
    payload.urls.push_back(url);
  }
  return payload;
}

inline bool IsCopyShortcut(const unsigned keyval, const unsigned modifiers, const unsigned control_mask) {
  return (modifiers & control_mask) != 0 && (keyval == 'c' || keyval == 'C');
}

}  // namespace PlaylistClipboard

#endif
