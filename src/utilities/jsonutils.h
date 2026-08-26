#ifndef STRAWBERRY_JSONUTILS_H
#define STRAWBERRY_JSONUTILS_H

#include "core/song.h"

#include <string>
#include <vector>

namespace JsonUtils {

bool LooksLikeImage(const std::string &data);
std::string StripHtml(const std::string &html);
std::string GetString(const std::string &json, const std::vector<std::string> &path);
std::string FindStringByKeys(const std::string &json, const std::vector<std::string> &keys);
std::string FindFirstImageUrl(const std::string &json);
std::string FindCoverUrl(const std::string &json);
std::string ExtractLyrics(const std::string &body);
SongList ParseSongs(const std::string &json, Song::Source source = Song::Source::Stream);
SongList ParseMusicBrainzRecordings(const std::string &json);
SongList ParseSubsonicSongs(const std::string &json);
SongList ParseSubsonicArtists(const std::string &json);
SongList ParseSubsonicAlbums(const std::string &json);
SongList ParseTidalTracks(const std::string &json);
SongList ParseTidalArtists(const std::string &json);
SongList ParseTidalAlbums(const std::string &json);
SongList ParseSpotifyTracks(const std::string &json);
SongList ParseSpotifyArtists(const std::string &json);
SongList ParseSpotifyAlbums(const std::string &json);
SongList ParseQobuzTracks(const std::string &json);
SongList ParseQobuzArtists(const std::string &json);
SongList ParseQobuzAlbums(const std::string &json);

}  // namespace JsonUtils

#endif
