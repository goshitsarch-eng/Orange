#ifndef STRAWBERRY_HTTPREQUESTREADER_H
#define STRAWBERRY_HTTPREQUESTREADER_H

#include <gio/gio.h>

#include <functional>
#include <string>

// Reads the request line of an HTTP request from a loopback connection without blocking.
//
// The OAuth redirect servers used to read with g_input_stream_read_all() and a fixed count, which only
// returns once that many bytes have arrived or the peer closes.  A browser sends a request far shorter than
// the buffer and then waits for the response, so the read sat there with the GTK main loop stopped behind
// it: the whole window froze until the browser gave up.
namespace HttpRequestReader {

// The request line, or an empty string if the request could not be read.  The connection is still open when
// the callback runs, so a response can be written to it; it is dropped afterwards.
using Callback = std::function<void(const std::string &request_line, GSocketConnection *connection)>;

// Caps what a single request may send before being abandoned, so a client that never sends a newline cannot
// make the process grow without bound.
inline constexpr size_t kMaxRequestBytes = 8192;

void ReadRequestLine(GSocketConnection *connection, Callback callback);

// "GET /callback?code=abc HTTP/1.1" -> "/callback?code=abc"
std::string TargetFromRequestLine(const std::string &request_line);

// "/callback?code=abc&state=xyz" -> the value of one query parameter, unescaped.
std::string QueryValue(const std::string &target, const std::string &key);

}  // namespace HttpRequestReader

#endif
