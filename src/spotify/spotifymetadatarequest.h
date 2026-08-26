#ifndef STRAWBERRY_SPOTIFYMETADATAREQUEST_H
#define STRAWBERRY_SPOTIFYMETADATAREQUEST_H

#include "core/network.h"
#include "core/song.h"

#include <functional>
#include <map>
#include <string>

namespace SpotifyMetadataRequest {

using Callback = std::function<void(const Song &, const std::string &error)>;

std::string TrackUrl(const std::string &api_url, const std::string &track_id);
std::string ArtistUrl(const std::string &api_url, const std::string &artist_id);
Song ParseTrack(const std::string &json);
std::string ParseArtistGenre(const std::string &json);

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Callback callback);

}  // namespace SpotifyMetadataRequest

#endif
