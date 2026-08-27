#include "playlistparsers/playlistparser.h"

#include "collection/collectionbackend.h"
#include "playlistparsers/asxiniparser.h"
#include "playlistparsers/asxparser.h"
#include "playlistparsers/cueparser.h"
#include "playlistparsers/m3uparser.h"
#include "playlistparsers/plsparser.h"
#include "playlistparsers/wplparser.h"
#include "playlistparsers/xspfparser.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>

PlaylistParser::PlaylistParser(CollectionBackend *backend) {
  ParserBase::SetCollectionBackend(backend);
  parsers_.push_back(std::make_unique<M3UParser>());
  parsers_.push_back(std::make_unique<PLSParser>());
  parsers_.push_back(std::make_unique<XSPFParser>());
  parsers_.push_back(std::make_unique<ASXParser>());
  parsers_.push_back(std::make_unique<AsxIniParser>());
  parsers_.push_back(std::make_unique<WplParser>());
  parsers_.push_back(std::make_unique<CueParser>());
}

ParserBase *PlaylistParser::ParserForExtension(const std::string &suffix) const {
  const std::string ext = StrUtils::ToLower(suffix);
  for (const auto &parser : parsers_) {
    const auto extensions = parser->file_extensions();
    if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
      return parser.get();
    }
  }
  return nullptr;
}

ParserBase *PlaylistParser::ParserForMagic(const std::string &data) const {
  for (const auto &parser : parsers_) {
    if (parser->TryMagic(data)) {
      return parser.get();
    }
  }
  return nullptr;
}

std::vector<ParserBase *> PlaylistParser::parsers() const {
  std::vector<ParserBase *> result;
  for (const auto &parser : parsers_) {
    result.push_back(parser.get());
  }
  return result;
}

bool PlaylistParser::IsPlaylist(const std::string &path) {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  const auto supported = SupportedExtensions();
  return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::vector<std::string> PlaylistParser::SupportedExtensions() { return {"m3u", "m3u8", "pls", "xspf", "asx", "asxini", "wpl", "cue"}; }

std::string PlaylistParser::FindCueForAudio(const std::string &audio_path) { return CueParser::FindCueFilename(audio_path); }

int64_t PlaylistParser::CueIndexToNanosec(const std::string &index) { return CueParser::IndexToNanosec(index); }

void PlaylistParser::EnrichFromAudioFile(SongList *songs, const Song &file) { CueParser::EnrichFromAudioFile(songs, file); }

SongList PlaylistParser::LoadFromData(const std::string &data, const std::string &hint) const {
  ParserBase *parser = ParserForExtension(FileUtils::Extension(hint));
  if (!parser) {
    parser = ParserForMagic(data);
  }
  if (!parser) {
    parser = ParserForExtension("m3u");
  }
  return parser ? parser->Load(data, hint) : SongList();
}

SongList PlaylistParser::Load(const std::string &path) const { return LoadFromData(FileUtils::ReadFile(path), path); }

bool PlaylistParser::Save(const std::string &path, const SongList &songs) const {
  ParserBase *parser = ParserForExtension(FileUtils::Extension(path));
  if (parser && parser->save_supported()) {
    return parser->Save(path, songs);
  }
  return M3UParser().Save(path, songs);
}
