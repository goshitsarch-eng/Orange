#ifndef STRAWBERRY_PLAYERINTERFACE_H
#define STRAWBERRY_PLAYERINTERFACE_H

#include "core/song.h"
#include "engine/enginebase.h"

#include <cstdint>

class PlayerInterface {
 public:
  virtual ~PlayerInterface() = default;
  virtual EngineBase::State GetState() const = 0;
  virtual unsigned GetVolume() const = 0;
  virtual const Song &current_song() const = 0;
  virtual void Play() = 0;
  virtual void PlayPause() = 0;
  virtual void Pause() = 0;
  virtual void Stop(bool stop_after = false) = 0;
  virtual void Next() = 0;
  virtual void Previous() = 0;
  virtual void SeekTo(int64_t seconds) = 0;
  virtual void SetVolume(unsigned volume) = 0;
};

#endif
