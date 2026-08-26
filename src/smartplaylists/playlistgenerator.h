#ifndef STRAWBERRY_PLAYLISTGENERATOR_H
#define STRAWBERRY_PLAYLISTGENERATOR_H

#include "core/song.h"

#include <memory>
#include <string>

class CollectionBackend;

class PlaylistGenerator {
 public:
  enum class Type { None = 0, Query = 1 };

  static const int kDefaultLimit = 100;
  static const int kDefaultDynamicHistory = 10;
  static const int kDefaultDynamicFuture = 10;

  virtual ~PlaylistGenerator() = default;

  static std::shared_ptr<PlaylistGenerator> Create(Type type = Type::Query);

  void set_collection_backend(CollectionBackend *backend) { collection_backend_ = backend; }
  void set_name(const std::string &name) { name_ = name; }
  CollectionBackend *collection() const { return collection_backend_; }
  const std::string &name() const { return name_; }

  virtual Type type() const = 0;
  virtual void Load(const std::string &data) = 0;
  virtual std::string Save() const = 0;
  virtual SongList Generate() = 0;
  virtual bool is_dynamic() const { return false; }
  virtual void set_dynamic(bool) {}
  virtual SongList GenerateMore(int count);
  virtual int GetDynamicHistory() const { return kDefaultDynamicHistory; }
  virtual int GetDynamicFuture() const { return kDefaultDynamicFuture; }

 protected:
  CollectionBackend *collection_backend_ = nullptr;

 private:
  std::string name_;
};

#endif
