#ifndef STRAWBERRY_GSTURL_H
#define STRAWBERRY_GSTURL_H

#include <string>

class GstUrl {
 public:
  std::string url;
  std::string source_device;

  // Qt GstEngine::FixupUrl: UNC file://host/path becomes file:////host/path.
  static bool IsUncFileUrl(const std::string &url) {
    return url.size() > 7 && url.compare(0, 7, "file://") == 0 && url[7] != '/';
  }

  static bool HasCddaScheme(const std::string &url) {
    return url.size() >= 5 && (url.compare(0, 5, "cdda:") == 0 || url.compare(0, 5, "CDDA:") == 0);
  }

  // Qt CddaSongLoader / GetUrlFromTrack: cdda://{device}/{track} when the drive is known.
  static std::string CddaSongUrl(int track, const std::string &device_path) {
    if (device_path.empty()) {
      return "cdda://" + std::to_string(track);
    }
    std::string device = device_path;
    if (device.front() != '/') {
      device.insert(device.begin(), '/');
    }
    return "cdda://" + device + "/" + std::to_string(track);
  }

  // Qt GstEngine::FixupUrl: GStreamer cannot embed the CD device in the URI.
  static GstUrl FixupCdda(const std::string &url) {
    GstUrl result;
    std::string rest = url.size() > 5 ? url.substr(5) : std::string();
    while (!rest.empty() && rest.front() == '/') {
      rest.erase(rest.begin());
    }
    const auto slash = rest.find_last_of('/');
    if (slash == std::string::npos) {
      result.url = "cdda://" + rest;
      return result;
    }
    result.url = "cdda://" + rest.substr(slash + 1);
    result.source_device = "/" + rest.substr(0, slash);
    return result;
  }

  static GstUrl Fixup(const std::string &url) {
    GstUrl result;
    result.url = url;
    if (IsUncFileUrl(url)) {
      result.url = "file:////" + url.substr(7);
      return result;
    }
    if (HasCddaScheme(url)) {
      return FixupCdda(url);
    }
    return result;
  }
};

#endif  // STRAWBERRY_GSTURL_H
