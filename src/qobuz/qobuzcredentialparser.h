#ifndef STRAWBERRY_QOBUZCREDENTIALPARSER_H
#define STRAWBERRY_QOBUZCREDENTIALPARSER_H

#include <glib.h>

#include <algorithm>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace QobuzCredentialParser {

inline const char *LoginPageUrl() { return "https://play.qobuz.com/login"; }
inline const char *PlayUrl() { return "https://play.qobuz.com"; }
inline const char *UserAgent() {
  return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
}
inline const char *MissingBundle() { return "Failed to find bundle.js URL in login page"; }
inline const char *MissingAppId() { return "Failed to extract app_id from bundle"; }
inline const char *MissingAppSecret() { return "Failed to extract app_secret from bundle"; }

inline std::string FailedLoginPage(const std::string &error) { return "Failed to fetch login page: " + error; }
inline std::string FailedBundle(const std::string &error) { return "Failed to fetch bundle.js: " + error; }

inline std::string BundleUrl(const std::string &path) { return std::string(PlayUrl()) + path; }

inline std::string FirstCapture(const std::string &text, const std::regex &pattern) {
  std::smatch match;
  if (std::regex_search(text, match, pattern) && match.size() > 1) {
    return match[1].str();
  }
  return {};
}

inline std::string ExtractBundlePath(const std::string &login_html) {
  static const std::regex pattern(R"regex(<script src="(/resources/[\d.]+-[a-z]\d+/bundle\.js)"></script>)regex");
  return FirstCapture(login_html, pattern);
}

inline std::string ExtractAppId(const std::string &bundle) {
  static const std::regex pattern(R"regex(production:\{api:\{appId:"(\d+)")regex");
  return FirstCapture(bundle, pattern);
}

inline std::string ExtractLoginAppId(const std::string &bundle) {
  static const std::regex pattern(R"regex(\{appId:"(\d{8,10})"\})regex");
  return FirstCapture(bundle, pattern);
}

inline std::string ExtractPrivateKey(const std::string &bundle) {
  static const std::regex pattern(R"regex(privateKey:"([A-Za-z0-9]+)")regex");
  return FirstCapture(bundle, pattern);
}

inline bool IsHexSecret(const std::string &value) {
  static const std::regex hex(R"regex(^[a-f0-9]{32}$)regex");
  return std::regex_match(value, hex);
}

inline std::string DecodeSecret(const std::string &seed, const std::string &info, const std::string &extras) {
  const std::string combined = seed + info + extras;
  if (combined.size() <= 44) {
    return {};
  }
  const std::string trimmed = combined.substr(0, combined.size() - 44);
  gsize decoded_len = 0;
  guchar *decoded = g_base64_decode(trimmed.c_str(), &decoded_len);
  std::string secret;
  if (decoded) {
    secret.assign(reinterpret_cast<char *>(decoded), decoded_len);
    g_free(decoded);
  }
  return IsHexSecret(secret) ? secret : std::string();
}

inline std::string ExtractAppSecret(const std::string &bundle) {
  static const std::regex seed_pattern(R"regex([a-z]\.initialSeed\("([\w=]+)",window\.utimezone\.([a-z]+)\))regex");
  static const std::regex info_pattern(R"regex(name:"\w+/(\w+)",info:"([\w=]+)",extras:"([\w=]+)")regex");
  std::map<std::string, std::string> seeds;
  for (std::sregex_iterator it(bundle.begin(), bundle.end(), seed_pattern), end; it != end; ++it) {
    seeds[(*it)[2].str()] = (*it)[1].str();
  }
  std::map<std::string, std::pair<std::string, std::string>> infos;
  for (std::sregex_iterator it(bundle.begin(), bundle.end(), info_pattern), end; it != end; ++it) {
    std::string tz = (*it)[1].str();
    std::transform(tz.begin(), tz.end(), tz.begin(), [](unsigned char ch) { return static_cast<char>(g_ascii_tolower(ch)); });
    infos[tz] = {(*it)[2].str(), (*it)[3].str()};
  }
  const std::vector<std::string> preferred = {"berlin", "london", "abidjan"};
  auto try_timezone = [&](const std::string &tz) -> std::string {
    const auto seed = seeds.find(tz);
    const auto info = infos.find(tz);
    if (seed == seeds.end() || info == infos.end()) {
      return {};
    }
    return DecodeSecret(seed->second, info->second.first, info->second.second);
  };
  for (const std::string &tz : preferred) {
    const std::string secret = try_timezone(tz);
    if (!secret.empty()) {
      return secret;
    }
  }
  for (const auto &seed : seeds) {
    if (std::find(preferred.begin(), preferred.end(), seed.first) != preferred.end()) {
      continue;
    }
    const std::string secret = try_timezone(seed.first);
    if (!secret.empty()) {
      return secret;
    }
  }
  return {};
}

}  // namespace QobuzCredentialParser

#endif
