#include "discord/discord.h"

#include "constants/notificationssettings.h"
#include "discord/discordart.h"
#include "discord/discordcover.h"
#include "discord/discordlifecycle.h"
#include "discord/discordreconnect.h"
#include "core/logging.h"
#include "core/settings.h"

#include <gio/gunixsocketaddress.h>

#include <cinttypes>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <vector>

namespace {

std::string PadArtist(const std::string &artist) {
  if (artist.empty()) {
    return {};
  }
  return artist.size() < 2 ? artist + " " : artist;
}

}  // namespace

DiscordRichPresence::DiscordRichPresence() = default;

DiscordRichPresence::~DiscordRichPresence() {
  shutting_down_ = true;
  CancelReconnect();
  Disconnect();
}

void DiscordRichPresence::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(DiscordRPCSettings::kSettingsGroup);
  enabled_ = settings.BoolValue(DiscordRPCSettings::kEnabled, DiscordRPCSettings::kDefaultEnabled);
  status_display_type_ = settings.IntValue(DiscordRPCSettings::kStatusDisplayType,
                                           static_cast<int>(DiscordRPCSettings::kDefaultStatusDisplayType));
  if (!enabled_) {
    CancelReconnect();
    Disconnect();
    return;
  }
  shutting_down_ = false;
}

std::string DiscordRichPresence::JsonEscape(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (unsigned char c : value) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out.push_back(static_cast<char>(c));
        }
        break;
    }
  }
  return out;
}

std::string DiscordRichPresence::HandshakeJson(const std::string &client_id) {
  return std::string(R"({"v":1,"client_id":")") + JsonEscape(client_id) + "\"}";
}

std::string DiscordRichPresence::SetActivityJson(const Song &song, bool playing, int status_display_type, gint64 start_timestamp,
                                                 int pid, unsigned nonce, const std::string &art_key) {
  const std::string title = JsonEscape(song.PrettyTitle());
  const std::string artist = JsonEscape(PadArtist(song.artist()));
  const std::string album = JsonEscape(song.album());
  const gint64 length_secs = song.length_nanosec() > 0 ? song.length_nanosec() / 1000000000LL : 0;
  const gint64 end_timestamp = start_timestamp > 0 && length_secs > 0 ? start_timestamp + length_secs : 0;
  const std::string large_image =
      art_key.empty() ? DiscordArt::ArtKey(DiscordArt::SongArtUrl(song.art_manual(), song.art_automatic())) : art_key;
  std::string json = "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"" + std::to_string(nonce) + "\",\"args\":{\"pid\":" +
                     std::to_string(pid) + ",\"activity\":{\"type\":2,\"status_display_type\":" + std::to_string(status_display_type) +
                     ",\"name\":\"Strawberry\",\"details\":\"" + title + "\",\"state\":\"" + artist +
                     "\",\"assets\":{\"large_image\":\"" + JsonEscape(large_image) +
                     "\",\"small_image\":\"embedded_cover\",\"small_text\":\"Strawberry Music Player\"";
  if (!album.empty()) {
    json += ",\"large_text\":\"on " + album + "\"";
  }
  json += "},\"instance\":false";
  if (playing && start_timestamp > 0) {
    json += ",\"timestamps\":{\"start\":" + std::to_string(start_timestamp);
    if (end_timestamp > 0) {
      json += ",\"end\":" + std::to_string(end_timestamp);
    }
    json += "}";
  }
  json += "}}}";
  return json;
}

std::string DiscordRichPresence::ClearActivityJson(int pid, unsigned nonce) {
  return std::string("{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"") + std::to_string(nonce) + "\",\"args\":{\"pid\":" +
         std::to_string(pid) + ",\"activity\":null}}";
}

std::string DiscordRichPresence::SocketPath(int index) {
  const std::vector<std::string> dirs = DiscordReconnect::TempDirs();
  if (dirs.empty()) {
    return DiscordReconnect::SocketPath("/tmp", index);
  }
  return DiscordReconnect::SocketPath(dirs.front(), index);
}

bool DiscordRichPresence::EnsureConnected() {
  if (connection_) {
    return true;
  }
  for (const std::string &path : DiscordReconnect::SocketPaths()) {
    GError *error = nullptr;
    GSocketAddress *address = g_unix_socket_address_new(path.c_str());
    GSocketClient *client = g_socket_client_new();
    GSocketConnection *conn = g_socket_client_connect(client, G_SOCKET_CONNECTABLE(address), nullptr, &error);
    g_object_unref(client);
    g_object_unref(address);
    if (!conn) {
      if (error) {
        g_error_free(error);
      }
      continue;
    }
    connection_ = conn;
    if (!SendFrame(Opcode::Handshake, HandshakeJson(kApplicationId))) {
      Disconnect();
      continue;
    }
    reconnect_delay_ = DiscordReconnect::kMinDelayMs;
    CancelReconnect();
    LogDebug("Discord IPC connected at %s", path.c_str());
    return true;
  }
  return false;
}

void DiscordRichPresence::Disconnect() {
  if (connection_) {
    g_io_stream_close(G_IO_STREAM(connection_), nullptr, nullptr);
    g_object_unref(connection_);
    connection_ = nullptr;
  }
}

void DiscordRichPresence::CancelReconnect() {
  if (reconnect_timer_) {
    g_source_remove(reconnect_timer_);
    reconnect_timer_ = 0;
  }
}

void DiscordRichPresence::ScheduleReconnect() {
  if (!DiscordReconnect::ShouldSchedule(enabled_, shutting_down_, reconnect_timer_ != 0)) {
    return;
  }
  reconnect_timer_ = g_timeout_add(static_cast<guint>(reconnect_delay_), +[](gpointer data) -> gboolean {
    static_cast<DiscordRichPresence *>(data)->OnReconnectTimer();
    return G_SOURCE_REMOVE;
  }, this);
  reconnect_delay_ = DiscordReconnect::NextDelay(reconnect_delay_);
}

void DiscordRichPresence::OnReconnectTimer() {
  reconnect_timer_ = 0;
  if (!enabled_ || shutting_down_) {
    return;
  }
  if (EnsureConnected()) {
    return;
  }
  ScheduleReconnect();
}

bool DiscordRichPresence::SendFrame(Opcode opcode, const std::string &payload) {
  if (!connection_) {
    return false;
  }
  guint32 header[2] = {static_cast<guint32>(opcode), static_cast<guint32>(payload.size())};
  GOutputStream *output = g_io_stream_get_output_stream(G_IO_STREAM(connection_));
  GError *error = nullptr;
  if (!g_output_stream_write_all(output, header, sizeof(header), nullptr, nullptr, &error) ||
      (!payload.empty() && !g_output_stream_write_all(output, payload.data(), payload.size(), nullptr, nullptr, &error))) {
    if (error) {
      LogWarning("Discord IPC write failed: %s", error->message);
      g_error_free(error);
    }
    Disconnect();
    ScheduleReconnect();
    return false;
  }
  return true;
}

void DiscordRichPresence::UpdatePresence(const Song &song, bool playing) {
  if (!enabled_) {
    return;
  }
  if (DiscordLifecycle::ShouldClear(playing)) {
    Clear();
    return;
  }
  if (!EnsureConnected()) {
    ScheduleReconnect();
    return;
  }
  if (start_timestamp_ <= 0) {
    start_timestamp_ = static_cast<gint64>(std::time(nullptr));
  }
  const std::string art_key = DiscordCover::ResolveArtKey(DiscordArt::SongArtUrl(song.art_manual(), song.art_automatic()));
  SendFrame(Opcode::Frame, SetActivityJson(song, true, status_display_type_, start_timestamp_, getpid(), nonce_++, art_key));
}

void DiscordRichPresence::RefreshAfterSeek(const Song &song, gint64 position_secs) {
  start_timestamp_ = DiscordLifecycle::StartTimestampAfterSeek(static_cast<gint64>(std::time(nullptr)), position_secs);
  UpdatePresence(song, true);
}

void DiscordRichPresence::Clear() {
  start_timestamp_ = 0;
  if (!enabled_ || !connection_) {
    return;
  }
  SendFrame(Opcode::Frame, ClearActivityJson(getpid(), nonce_++));
}
