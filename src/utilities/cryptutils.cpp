#include "utilities/cryptutils.h"

#include <glib.h>

#include <cstdio>

namespace CryptUtils {

std::string HexEncode(const std::string &data) {
  std::string out;
  out.reserve(data.size() * 2);
  for (unsigned char ch : data) {
    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02x", ch);
    out += buf;
  }
  return out;
}

std::string HmacSha1(const std::string &key, const std::string &data) {
  GHmac *hmac = g_hmac_new(G_CHECKSUM_SHA1, reinterpret_cast<const guchar *>(key.data()), key.size());
  g_hmac_update(hmac, reinterpret_cast<const guchar *>(data.data()), data.size());
  gsize len = 20;
  guint8 digest[20];
  g_hmac_get_digest(hmac, digest, &len);
  g_hmac_unref(hmac);
  return std::string(reinterpret_cast<char *>(digest), len);
}

std::string HmacSha256(const std::string &key, const std::string &data) {
  GHmac *hmac = g_hmac_new(G_CHECKSUM_SHA256, reinterpret_cast<const guchar *>(key.data()), key.size());
  g_hmac_update(hmac, reinterpret_cast<const guchar *>(data.data()), data.size());
  gsize len = 32;
  guint8 digest[32];
  g_hmac_get_digest(hmac, digest, &len);
  g_hmac_unref(hmac);
  return std::string(reinterpret_cast<char *>(digest), len);
}

}  // namespace CryptUtils
