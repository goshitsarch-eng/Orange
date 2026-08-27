#ifndef STRAWBERRY_NETWORKTIMEOUTS_H
#define STRAWBERRY_NETWORKTIMEOUTS_H

#include <functional>
#include <map>

class NetworkAccessManager;

class NetworkTimeouts {
 public:
  using Abort = std::function<void(int)>;

  NetworkTimeouts() = default;
  ~NetworkTimeouts() { CancelAll(); }

  void SetTimeout(int msec) { timeout_msec_ = msec; }
  int timeout() const { return timeout_msec_; }
  void SetAbort(Abort abort) { abort_ = std::move(abort); }

  void AddReply(int id);
  void Cancel(int id);
  void CancelAll();
  bool Contains(int id) const;

  // Qt NetworkTimeouts::AddReply(QNetworkReply*): abort the Soup request on expiry.
  void Watch(NetworkAccessManager *network, int id);

 private:
  int timeout_msec_ = 5000;
  std::map<int, unsigned> timers_;
  Abort abort_;
};

#endif
