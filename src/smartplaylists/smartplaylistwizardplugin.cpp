#include "smartplaylists/smartplaylistwizardplugin.h"

#include "smartplaylists/playlistquerygenerator.h"

std::shared_ptr<PlaylistGenerator> SmartPlaylistQueryWizardPlugin::CreateGenerator(const std::string &name, bool dynamic) const {
  return std::make_shared<PlaylistQueryGenerator>(name, search_, dynamic);
}
