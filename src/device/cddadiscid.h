#ifndef STRAWBERRY_CDDADISCID_H
#define STRAWBERRY_CDDADISCID_H

#include <glib.h>

#include <cstdio>
#include <string>
#include <vector>

namespace CddaDiscId {

// MusicBrainz TOC: first(2) + last(2) + leadout(8) + 99 offsets(8) as uppercase hex.
inline std::string TocString(int first, int last, int leadout, const std::vector<int> &offsets) {
  std::string toc;
  toc.reserve(12 + 99 * 8);
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%02X%02X%08X", first & 0xff, last & 0xff, static_cast<unsigned>(leadout));
  toc.append(buf);
  for (int i = 0; i < 99; ++i) {
    const int off = i < static_cast<int>(offsets.size()) ? offsets[static_cast<size_t>(i)] : 0;
    std::snprintf(buf, sizeof(buf), "%08X", static_cast<unsigned>(off));
    toc.append(buf);
  }
  return toc;
}

// SHA-1 then MusicBrainz base64: + → .  / → _  = → -
inline std::string EncodeSha1(const std::string &toc) {
  GChecksum *sum = g_checksum_new(G_CHECKSUM_SHA1);
  g_checksum_update(sum, reinterpret_cast<const guchar *>(toc.data()), static_cast<gssize>(toc.size()));
  guint8 digest[20];
  gsize len = sizeof(digest);
  g_checksum_get_digest(sum, digest, &len);
  g_checksum_free(sum);
  gchar *b64 = g_base64_encode(digest, len);
  std::string encoded = b64 ? b64 : "";
  g_free(b64);
  for (char &c : encoded) {
    if (c == '+') {
      c = '.';
    } else if (c == '/') {
      c = '_';
    } else if (c == '=') {
      c = '-';
    }
  }
  return encoded;
}

inline std::string FromOffsets(int first, int last, int leadout, const std::vector<int> &offsets) {
  if (first <= 0 || last < first) {
    return {};
  }
  return EncodeSha1(TocString(first, last, leadout, offsets));
}

}  // namespace CddaDiscId

#endif
