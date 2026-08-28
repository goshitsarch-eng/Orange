#include "core/httprequestreader.h"

#include "utilities/strutils.h"

#include <glib.h>

namespace HttpRequestReader {

namespace {

struct ReadState {
  GSocketConnection *connection = nullptr;
  Callback callback;
  std::string buffer;
  char chunk[1024] = {};
};

void Finish(ReadState *state, const std::string &request_line) {
  if (state->callback) {
    state->callback(request_line, state->connection);
  }
  g_object_unref(state->connection);
  delete state;
}

void ReadChunk(ReadState *state);

void OnRead(GObject *source, GAsyncResult *result, gpointer data) {
  auto *state = static_cast<ReadState *>(data);
  const gssize read = g_input_stream_read_finish(G_INPUT_STREAM(source), result, nullptr);
  if (read <= 0) {
    // End of stream or an error: whatever arrived is all there will be.
    Finish(state, {});
    return;
  }
  state->buffer.append(state->chunk, static_cast<size_t>(read));
  const size_t end = state->buffer.find("\r\n");
  if (end != std::string::npos) {
    Finish(state, state->buffer.substr(0, end));
    return;
  }
  const size_t newline = state->buffer.find('\n');
  if (newline != std::string::npos) {
    Finish(state, state->buffer.substr(0, newline));
    return;
  }
  if (state->buffer.size() >= kMaxRequestBytes) {
    Finish(state, {});
    return;
  }
  ReadChunk(state);
}

void ReadChunk(ReadState *state) {
  GInputStream *input = g_io_stream_get_input_stream(G_IO_STREAM(state->connection));
  g_input_stream_read_async(input, state->chunk, sizeof(state->chunk), G_PRIORITY_DEFAULT, nullptr, OnRead, state);
}

}  // namespace

void ReadRequestLine(GSocketConnection *connection, Callback callback) {
  if (!connection) {
    if (callback) {
      callback({}, nullptr);
    }
    return;
  }
  auto *state = new ReadState;
  state->connection = G_SOCKET_CONNECTION(g_object_ref(connection));
  state->callback = std::move(callback);
  ReadChunk(state);
}

std::string TargetFromRequestLine(const std::string &request_line) {
  const size_t first = request_line.find(' ');
  if (first == std::string::npos) {
    return {};
  }
  const size_t second = request_line.find(' ', first + 1);
  return request_line.substr(first + 1, second == std::string::npos ? std::string::npos : second - (first + 1));
}

std::string QueryValue(const std::string &target, const std::string &key) {
  const size_t question = target.find('?');
  if (question == std::string::npos) {
    return {};
  }
  const std::string query = target.substr(question + 1);
  const std::string prefix = key + "=";
  size_t pos = 0;
  while (pos < query.size()) {
    const size_t amp = query.find('&', pos);
    const std::string part = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    if (StrUtils::StartsWith(part, prefix)) {
      gchar *unescaped = g_uri_unescape_string(part.substr(prefix.size()).c_str(), nullptr);
      std::string value = unescaped ? unescaped : part.substr(prefix.size());
      g_free(unescaped);
      return value;
    }
    if (amp == std::string::npos) {
      break;
    }
    pos = amp + 1;
  }
  return {};
}

}  // namespace HttpRequestReader
