#ifndef STRAWBERRY_SCOPEDFLAG_H
#define STRAWBERRY_SCOPEDFLAG_H

class ScopedFlag {
 public:
  explicit ScopedFlag(bool *flag) : flag_(flag) {
    if (flag_) {
      *flag_ = true;
    }
  }
  ~ScopedFlag() {
    if (flag_) {
      *flag_ = false;
    }
  }

 private:
  bool *flag_ = nullptr;
};

#endif
