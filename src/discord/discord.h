#ifndef STRAWBERRY_DISCORD_H
#define STRAWBERRY_DISCORD_H

#include "core/song.h"

#include <gio/gio.h>

#include <string>

class DiscordRichPresence {
 public:
  static constexpr const char *kApplicationId = "1352351827206733974";

  DiscordRichPresence();
  ~DiscordRichPresence();

  void ReloadSettings();
  void UpdatePresence(const Song &song, bool playing);
  void Clear();
  bool enabled() const { return enabled_; }
  bool connected() const { return connection_ != nullptr; }

  static std::string JsonEscape(const std::string &value);
  static std::string HandshakeJson(const std::string &client_id);
  static std::string SetActivityJson(const Song &song, bool playing, int status_display_type, gint64 start_timestamp, int pid,
                                    unsigned nonce, const std::string &art_key = {});
  static std::string ClearActivityJson(int pid, unsigned nonce);
  static std::string SocketPath(int index);

 private:
  enum class Opcode { Handshake = 0, Frame = 1, Close = 2 };

  bool EnsureConnected();
  void Disconnect();
  bool SendFrame(Opcode opcode, const std::string &payload);

  bool enabled_ = false;
  int status_display_type_ = 0;
  unsigned nonce_ = 1;
  gint64 start_timestamp_ = 0;
  GSocketConnection *connection_ = nullptr;
};

#endif
