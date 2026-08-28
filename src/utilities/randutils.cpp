#include "utilities/randutils.h"

#include <gio/gio.h>
#include <glib.h>

#include <cstdint>
#include <vector>

namespace RandUtils {

namespace {

// g_random_* is a Mersenne Twister: fine for shuffling a playlist, useless for anything an attacker may try
// to predict.  GRand is seeded from a small amount of entropy and its whole state can be recovered from a
// short run of output, so PKCE verifiers and authentication salts have to come from the system CSPRNG.
bool FillSecureBytes(uint8_t *out, size_t count) {
  GError *error = nullptr;
  GFile *urandom = g_file_new_for_path("/dev/urandom");
  GFileInputStream *stream = g_file_read(urandom, nullptr, &error);
  g_object_unref(urandom);
  if (!stream) {
    if (error) {
      g_error_free(error);
    }
    return false;
  }
  gsize read = 0;
  const gboolean ok = g_input_stream_read_all(G_INPUT_STREAM(stream), out, count, &read, nullptr, &error);
  g_object_unref(stream);
  if (error) {
    g_error_free(error);
  }
  return ok == TRUE && read == count;
}

}  // namespace

std::string GetRandomString(int len, const char *alphabet) {
  std::string out;
  out.reserve(len);
  const int n = static_cast<int>(g_utf8_strlen(alphabet, -1));
  for (int i = 0; i < len; ++i) {
    out.push_back(alphabet[g_random_int_range(0, n)]);
  }
  return out;
}

std::string GetRandomStringWithChars(int len) { return GetRandomString(len, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"); }

std::string GetRandomStringWithCharsAndNumbers(int len) {
  return GetRandomString(len, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

std::string CryptographicRandomString(int len) {
  static const char kAlphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  constexpr size_t kAlphabetSize = sizeof(kAlphabet) - 1;
  if (len <= 0) {
    return {};
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(len));
  if (!FillSecureBytes(bytes.data(), bytes.size())) {
    // Nothing here is safe to fall back to a predictable generator for, so say so rather than quietly
    // handing out a guessable secret.
    return {};
  }
  std::string out;
  out.reserve(static_cast<size_t>(len));
  for (uint8_t byte : bytes) {
    // 256 is not a multiple of 62, so the plain modulo would favour the first few characters.  Redraw the
    // values in the biased tail instead.
    uint8_t value = byte;
    while (value >= (256 / kAlphabetSize) * kAlphabetSize) {
      if (!FillSecureBytes(&value, 1)) {
        return {};
      }
    }
    out.push_back(kAlphabet[value % kAlphabetSize]);
  }
  return out;
}

}  // namespace RandUtils
