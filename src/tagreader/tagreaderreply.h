#ifndef STRAWBERRY_TAGREADERREPLY_H
#define STRAWBERRY_TAGREADERREPLY_H

#include "core/signal.h"
#include "tagreader/tagreaderresult.h"

#include <memory>
#include <string>

class TagReaderReply {
 public:
  explicit TagReaderReply(const std::string &filename) : filename_(filename) {}
  virtual ~TagReaderReply() = default;

  const std::string &filename() const { return filename_; }
  TagReaderResult result() const { return result_; }
  void set_result(const TagReaderResult &result) { result_ = result; }
  bool finished() const { return finished_; }
  bool success() const { return result_.success(); }
  std::string error() const { return result_.error_string(); }

  virtual void Finish() {
    finished_ = true;
    Finished.Emit(filename_, result_);
  }

  Signal<std::string, TagReaderResult> Finished;

 protected:
  std::string filename_;
  bool finished_ = false;
  TagReaderResult result_;
};

using TagReaderReplyPtr = std::shared_ptr<TagReaderReply>;

#endif
