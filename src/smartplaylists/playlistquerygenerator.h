#ifndef STRAWBERRY_PLAYLISTQUERYGENERATOR_H
#define STRAWBERRY_PLAYLISTQUERYGENERATOR_H

#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/smartplaylist.h"

#include <vector>

class PlaylistQueryGenerator : public PlaylistGenerator {
 public:
  PlaylistQueryGenerator() = default;
  PlaylistQueryGenerator(const std::string &name, const SmartPlaylistSearch &search, bool dynamic = false);

  Type type() const override { return Type::Query; }

  void Load(const SmartPlaylistSearch &search);
  void Load(const std::string &data) override;
  std::string Save() const override;

  SongList Generate() override;
  SongList GenerateMore(int count) override;
  bool is_dynamic() const override { return dynamic_; }
  void set_dynamic(bool dynamic) override { dynamic_ = dynamic; }

  const SmartPlaylistSearch &search() const { return search_; }
  int GetDynamicFuture() const override { return search_.limit > 0 ? search_.limit : kDefaultDynamicFuture; }

 private:
  SmartPlaylistSearch search_;
  bool dynamic_ = false;
  std::vector<std::string> previous_urls_;
};

#endif
