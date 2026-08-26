#ifndef STRAWBERRY_ALBUMCOVERBATCH_H
#define STRAWBERRY_ALBUMCOVERBATCH_H

#include "core/song.h"

#include <string>
#include <vector>

class AlbumCoverBatch {
 public:
  struct Job {
    std::string artist;
    std::string album;
    Song song;
  };

  void Reset() {
    jobs_.clear();
    index_ = 0;
    succeeded_ = 0;
    failed_ = 0;
    cancelled_ = false;
    started_ = false;
  }

  void Enqueue(Job job) { jobs_.push_back(std::move(job)); }

  void Start() { started_ = true; }

  const Job *Current() const {
    if (index_ >= jobs_.size()) {
      return nullptr;
    }
    return &jobs_[index_];
  }

  void MarkSuccess() {
    if (index_ < jobs_.size()) {
      ++succeeded_;
      ++index_;
    }
  }

  void MarkFailure() {
    if (index_ < jobs_.size()) {
      ++failed_;
      ++index_;
    }
  }

  void Cancel() { cancelled_ = true; }

  bool started() const { return started_; }
  bool cancelled() const { return cancelled_; }
  bool finished() const { return started_ && (cancelled_ || index_ >= jobs_.size()); }
  bool running() const { return started_ && !finished(); }

  size_t total() const { return jobs_.size(); }
  size_t succeeded() const { return succeeded_; }
  size_t failed() const { return failed_; }
  size_t completed() const { return succeeded_ + failed_; }
  size_t remaining() const { return index_ >= jobs_.size() ? 0 : jobs_.size() - index_; }

  double Progress() const {
    if (jobs_.empty()) {
      return 1.0;
    }
    return static_cast<double>(completed()) / static_cast<double>(jobs_.size());
  }

  std::string StatusText() const {
    if (jobs_.empty()) {
      return "No albums to fetch.";
    }
    if (cancelled_) {
      return "Fetch cancelled (" + std::to_string(succeeded_) + " saved, " + std::to_string(failed_) + " failed).";
    }
    if (finished()) {
      return "Fetch finished (" + std::to_string(succeeded_) + " saved, " + std::to_string(failed_) + " failed).";
    }
    return "Fetching covers: " + std::to_string(completed()) + "/" + std::to_string(jobs_.size()) + " (" +
           std::to_string(succeeded_) + " saved, " + std::to_string(failed_) + " failed)";
  }

 private:
  std::vector<Job> jobs_;
  size_t index_ = 0;
  size_t succeeded_ = 0;
  size_t failed_ = 0;
  bool cancelled_ = false;
  bool started_ = false;
};

#endif
