#include "covermanager/coverutils.h"

#include "utilities/jsonutils.h"

std::string CoverUtils::ExtensionForData(const std::vector<unsigned char> &data) {
  return ExtensionForData(std::string(data.begin(), data.end()));
}

std::string CoverUtils::ExtensionForData(const std::string &data) {
  if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xff && static_cast<unsigned char>(data[1]) == 0xd8) {
    return "jpg";
  }
  if (data.size() >= 8 && data.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0) {
    return "png";
  }
  if (data.size() >= 4 && data.compare(0, 4, "RIFF") == 0) {
    return "webp";
  }
  return "jpg";
}

bool CoverUtils::LooksLikeImage(const std::string &data) { return JsonUtils::LooksLikeImage(data); }
