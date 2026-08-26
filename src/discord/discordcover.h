#ifndef STRAWBERRY_DISCORDCOVER_H
#define STRAWBERRY_DISCORDCOVER_H

#include "discord/discordart.h"
#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace DiscordCover {

inline bool NeedsUpload(const std::string &art_url) { return !art_url.empty() && !DiscordArt::IsHttpUrl(art_url); }

inline std::string PathFromArtUrl(const std::string &url) {
  if (url.rfind("file://", 0) == 0) {
    std::string path = url.substr(7);
    if (path.rfind("//", 0) == 0) {
      const size_t slash = path.find('/', 2);
      path = slash == std::string::npos ? path : path.substr(slash);
    } else if (!path.empty() && path[0] != '/') {
      const size_t slash = path.find('/');
      path = slash == std::string::npos ? std::string() : path.substr(slash);
    }
    return path;
  }
  if (!url.empty() && url[0] == '/') {
    return url;
  }
  return {};
}

inline std::string MimeFromPath(const std::string &path) {
  const std::string::size_type slash = path.rfind('/');
  const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
  const std::string::size_type dot = name.rfind('.');
  if (dot == std::string::npos || dot + 1 >= name.size()) {
    return "image/jpeg";
  }
  const std::string ext = StrUtils::ToLower(name.substr(dot + 1));
  if (ext == "png") {
    return "image/png";
  }
  if (ext == "webp") {
    return "image/webp";
  }
  if (ext == "gif") {
    return "image/gif";
  }
  if (ext == "bmp") {
    return "image/bmp";
  }
  return "image/jpeg";
}

inline std::string Base64Encode(const std::vector<unsigned char> &data) {
  static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  for (size_t i = 0; i < data.size(); i += 3) {
    const unsigned int n = (static_cast<unsigned int>(data[i]) << 16) | ((i + 1 < data.size() ? data[i + 1] : 0) << 8) |
                           (i + 2 < data.size() ? data[i + 2] : 0);
    out.push_back(kTable[(n >> 18) & 63]);
    out.push_back(kTable[(n >> 12) & 63]);
    out.push_back(i + 1 < data.size() ? kTable[(n >> 6) & 63] : '=');
    out.push_back(i + 2 < data.size() ? kTable[n & 63] : '=');
  }
  return out;
}

inline std::string DataUrl(const std::string &mime, const std::vector<unsigned char> &data) {
  if (data.empty()) {
    return {};
  }
  return "data:" + mime + ";base64," + Base64Encode(data);
}

}  // namespace DiscordCover

#endif
