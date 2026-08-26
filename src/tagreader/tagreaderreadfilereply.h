#ifndef STRAWBERRY_TAGREADERREADFILEREPLY_H
#define STRAWBERRY_TAGREADERREADFILEREPLY_H

#include "core/song.h"
#include "tagreader/tagreaderreply.h"

#include <memory>

class TagReaderReadFileReply : public TagReaderReply {
 public:
  explicit TagReaderReadFileReply(const std::string &filename) : TagReaderReply(filename) {}

  Song song() const { return song_; }
  void set_song(const Song &song) { song_ = song; }

  void Finish() override {
    finished_ = true;
    SongFinished.Emit(filename_, song_, result_);
    TagReaderReply::Finished.Emit(filename_, result_);
  }

  Signal<std::string, Song, TagReaderResult> SongFinished;

 private:
  Song song_;
};

using TagReaderReadFileReplyPtr = std::shared_ptr<TagReaderReadFileReply>;

#endif
