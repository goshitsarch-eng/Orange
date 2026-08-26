#ifndef STRAWBERRY_SMARTPLAYLISTWIZARDPLUGIN_H
#define STRAWBERRY_SMARTPLAYLISTWIZARDPLUGIN_H

#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/smartplaylist.h"

#include <memory>
#include <string>

class SmartPlaylistWizardPlugin {
 public:
  virtual ~SmartPlaylistWizardPlugin() = default;
  virtual std::string name() const = 0;
  virtual PlaylistGenerator::Type type() const = 0;
  virtual std::shared_ptr<PlaylistGenerator> CreateGenerator(const std::string &name, bool dynamic) const = 0;
};

class SmartPlaylistQueryWizardPlugin : public SmartPlaylistWizardPlugin {
 public:
  explicit SmartPlaylistQueryWizardPlugin(SmartPlaylistSearch search) : search_(std::move(search)) {}
  std::string name() const override { return "Query"; }
  PlaylistGenerator::Type type() const override { return PlaylistGenerator::Type::Query; }
  std::shared_ptr<PlaylistGenerator> CreateGenerator(const std::string &name, bool dynamic) const override;
  const SmartPlaylistSearch &search() const { return search_; }

 private:
  SmartPlaylistSearch search_;
};

#endif
