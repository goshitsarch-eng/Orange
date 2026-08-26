#ifndef STRAWBERRY_THREADSAFENETWORKDISKCACHE_H
#define STRAWBERRY_THREADSAFENETWORKDISKCACHE_H

#include <mutex>
#include <string>
#include <unordered_map>

class ThreadSafeNetworkDiskCache {
 public:
  void Insert(const std::string &key, const std::string &data);
  std::string Value(const std::string &key) const;
  void Clear();

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::string> cache_;
};

#endif
