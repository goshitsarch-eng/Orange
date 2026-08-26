#ifndef STRAWBERRY_NETWORKTIMEOUTS_H
#define STRAWBERRY_NETWORKTIMEOUTS_H

#include <map>

class NetworkTimeouts {
 public:
  void SetTimeout(int msec) { timeout_msec_ = msec; }
  int timeout() const { return timeout_msec_; }
  void AddReply(int id);
  void Cancel(int id);
  void CancelAll();
  bool Contains(int id) const;

 private:
  int timeout_msec_ = 5000;
  std::map<int, unsigned> timers_;
};

#endif
