#include "core/threadsafenetworkdiskcache.h"

void ThreadSafeNetworkDiskCache::Insert(const std::string &key, const std::string &data) {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_[key] = data;
}

std::string ThreadSafeNetworkDiskCache::Value(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = cache_.find(key);
  return it == cache_.end() ? std::string() : it->second;
}

void ThreadSafeNetworkDiskCache::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  cache_.clear();
}
