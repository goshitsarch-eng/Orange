#ifndef STRAWBERRY_ENGINEBASE_H
#define STRAWBERRY_ENGINEBASE_H

#include "core/signal.h"
#include "core/song.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class EngineBase {
 public:
  enum class State { Empty, Idle, Playing, Paused, Error };

  enum TrackChangeType { First = 0x01, Manual = 0x02, Auto = 0x04, Intro = 0x08, SameAlbum = 0x10 };

  struct OutputDetails {
    std::string name;
    std::string description;
    std::string iconname;
  };

  using Scope = std::vector<int16_t>;

  virtual ~EngineBase() = default;

  virtual bool Init() = 0;
  virtual State state() const = 0;
  virtual void StartPreloading(const std::string &media_url, const std::string &stream_url, bool force_stop_at_end,
                               int64_t beginning_offset_nanosec, int64_t end_offset_nanosec);
  virtual bool Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool force_stop_at_end,
                    uint64_t beginning_offset_nanosec, int64_t end_offset_nanosec, std::optional<double> ebur128_lufs) = 0;
  virtual bool Play(bool pause, uint64_t offset_nanosec) = 0;
  virtual void Stop(bool stop_after = false) = 0;
  virtual void Pause() = 0;
  virtual void Unpause() = 0;
  virtual void Seek(uint64_t offset_nanosec) = 0;
  virtual void SetVolumeSW(unsigned percent) = 0;
  virtual int64_t position_nanosec() const = 0;
  virtual int64_t length_nanosec() const = 0;
  virtual const Scope &scope() const { return scope_; }

  // Qt EngineBase::UpdateSpotifyAccessToken — stored for spotifyaudiosrc access-token.
  void UpdateSpotifyAccessToken(const std::string &token);
  const std::string &spotify_access_token() const { return spotify_access_token_; }

  Signal<State> StateChanged;
  Signal<int64_t, int64_t> PositionChanged;
  Signal<> TrackEnded;
  Signal<> TrackAboutToEnd;
  Signal<std::string> Error;
  Signal<> FatalError;
  Signal<std::string> InvalidSongRequested;
  Signal<std::string> ValidSongRequested;
  Signal<Song> MetadataReceived;
  Signal<> Finished;
  Signal<unsigned> VolumeChanged;

 protected:
  virtual void SetSpotifyAccessToken() {}

  Scope scope_;
  std::string spotify_access_token_;
};

#endif
