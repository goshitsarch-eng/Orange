#ifndef STRAWBERRY_PLAYLISTITEM_H
#define STRAWBERRY_PLAYLISTITEM_H

#include "core/song.h"
#include "playlist/playlistitemsavedata.h"

#include <memory>
#include <string>
#include <vector>

class PlaylistItem {
 public:
  enum class Option {
    Default = 0x00,
    PauseDisabled = 0x01,
    SeekDisabled = 0x04
  };

  explicit PlaylistItem(Song::Source source, const std::string &uuid = {});
  virtual ~PlaylistItem() = default;

  static std::shared_ptr<PlaylistItem> NewFromSource(Song::Source source, const std::string &uuid = {});
  static std::shared_ptr<PlaylistItem> NewFromSong(const Song &song);

  const std::string &uuid() const { return uuid_; }
  void set_uuid(const std::string &uuid) { uuid_ = uuid; }
  bool uuid_generated() const { return uuid_generated_; }

  virtual Song::Source source() const { return source_; }
  virtual Option options() const { return Option::Default; }

  virtual Song OriginalMetadata() const = 0;
  virtual std::string OriginalUrl() const = 0;
  virtual void SetOriginalMetadata(const Song &song) { (void)song; }

  Song EffectiveMetadata() const;
  std::string EffectiveUrl() const;

  void SetStreamMetadata(const Song &song);
  void UpdateStreamMetadata(const Song &song);
  void ClearStreamMetadata();
  bool HasStreamMetadata() const { return stream_song_.is_valid(); }

  virtual void SetArtManual(const std::string &cover_url) = 0;
  virtual bool IsLocalCollectionItem() const { return false; }

  void SetShouldSkip(bool should_skip) { should_skip_ = should_skip; }
  bool GetShouldSkip() const { return should_skip_; }

  unsigned long long save_generation() const { return save_generation_; }
  unsigned long long BumpSaveGeneration() { return ++save_generation_; }

  PlaylistItemSaveData CreateSaveData() const;

 protected:
  virtual Song DatabaseSongMetadata() const { return OriginalMetadata(); }

  Song::Source source_;
  std::string uuid_;
  bool uuid_generated_ = false;
  Song stream_song_;
  bool should_skip_ = false;
  unsigned long long save_generation_ = 0;
};

using PlaylistItemPtr = std::shared_ptr<PlaylistItem>;
using PlaylistItemPtrList = std::vector<PlaylistItemPtr>;

#endif
