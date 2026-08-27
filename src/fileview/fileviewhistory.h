#ifndef STRAWBERRY_FILEVIEWHISTORY_H
#define STRAWBERRY_FILEVIEWHISTORY_H

#include <string>
#include <vector>

class FileViewHistory {
 public:
  void Push(const std::string &path) {
    if (path.empty()) {
      return;
    }
    if (index_ >= 0 && index_ < static_cast<int>(items_.size()) && items_[static_cast<size_t>(index_)] == path) {
      return;
    }
    if (index_ + 1 < static_cast<int>(items_.size())) {
      items_.resize(static_cast<size_t>(index_ + 1));
    }
    items_.push_back(path);
    index_ = static_cast<int>(items_.size()) - 1;
  }

  bool CanBack() const { return index_ > 0; }
  bool CanForward() const { return index_ >= 0 && index_ + 1 < static_cast<int>(items_.size()); }

  std::string Back() {
    if (!CanBack()) {
      return Current();
    }
    --index_;
    return Current();
  }

  std::string Forward() {
    if (!CanForward()) {
      return Current();
    }
    ++index_;
    return Current();
  }

  std::string Current() const {
    if (index_ < 0 || index_ >= static_cast<int>(items_.size())) {
      return {};
    }
    return items_[static_cast<size_t>(index_)];
  }

  int index() const { return index_; }
  const std::vector<std::string> &items() const { return items_; }

 private:
  std::vector<std::string> items_;
  int index_ = -1;
};

#endif
