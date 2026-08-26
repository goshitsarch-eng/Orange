#ifndef STRAWBERRY_TAGREADERREADSTREAMREPLY_H
#define STRAWBERRY_TAGREADERREADSTREAMREPLY_H

#include "core/song.h"
#include "tagreader/tagreaderreply.h"

#include <memory>

class TagReaderReadStreamReply : public TagReaderReply {
 public:
  TagReaderReadStreamReply(const std::string &url, const std::string &filename) : TagReaderReply(filename), url_(url) {}

  const std::string &url() const { return url_; }
  Song song() const { return song_; }
  void set_song(const Song &song) { song_ = song; }

  void Finish() override {
    finished_ = true;
    StreamFinished.Emit(filename_, song_, result_);
    TagReaderReply::Finished.Emit(filename_, result_);
  }

  Signal<std::string, Song, TagReaderResult> StreamFinished;

 private:
  std::string url_;
  Song song_;
};

using TagReaderReadStreamReplyPtr = std::shared_ptr<TagReaderReadStreamReply>;

#endif
