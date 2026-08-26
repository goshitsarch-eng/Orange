#include "tagfetcher/tagfetcher.h"

#include "config.h"
#include "core/logging.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"

#ifdef HAVE_CHROMAPRINT
#include "engine/chromaprinter.h"
#endif

TagFetcher::TagFetcher(NetworkAccessManager *network)
    : network_(network), acoustid_(std::make_unique<AcoustidClient>(network)), musicbrainz_(std::make_unique<MusicBrainzClient>(network)) {
  acoustid_->Finished.Connect([this](int, const std::vector<std::string> &mbids, const std::string &error) {
    if (!error.empty() || mbids.empty()) {
      Results.Emit({});
      return;
    }
    musicbrainz_->Start(1, mbids);
  });
  musicbrainz_->Finished.Connect([this](int, const MusicBrainzClient::ResultList &results, const std::string &) {
    Results.Emit(MusicBrainzClient::ToSongs(results));
  });
}

void TagFetcher::FetchByMetadata(const Song &song) {
  if (!network_) {
    Results.Emit({});
    return;
  }
  std::string query = song.artist() + " " + song.title();
  if (query.empty()) {
    query = FileUtils::BaseName(FileUtils::PathFromUri(song.url()));
  }
  gchar *escaped = g_uri_escape_string(query.c_str(), nullptr, TRUE);
  const std::string url = std::string("https://musicbrainz.org/ws/2/recording/?query=") + (escaped ? escaped : "") + "&fmt=json";
  g_free(escaped);
  network_->Get(url, [this, song](const NetworkAccessManager::Response &response) {
    SongList results;
    if (response.ok()) {
      results = JsonUtils::ParseMusicBrainzRecordings(response.body);
      for (Song &result : results) {
        result.set_url(song.url());
        result.set_id(song.id());
      }
    }
    Results.Emit(results);
  }, {{"User-Agent", "Strawberry/1.0 (https://www.strawberrymusicplayer.org/)"}});
}

void TagFetcher::FetchByFingerprint(const Song &song) {
#ifdef HAVE_CHROMAPRINT
  Chromaprinter printer(song.url());
  const std::string fingerprint = printer.CreateFingerprint();
  if (fingerprint.empty()) {
    FetchByMetadata(song);
    return;
  }
  const int duration_msec = song.length_nanosec() > 0 ? static_cast<int>(song.length_nanosec() / 1000000) : 30000;
  acoustid_->Start(1, fingerprint, duration_msec);
#else
  FetchByMetadata(song);
#endif
}

void TagFetcher::Fetch(const Song &song) {
#ifdef HAVE_CHROMAPRINT
  LogInfo("Fingerprinting %s via AcoustID/MusicBrainz", song.url().c_str());
  FetchByFingerprint(song);
#else
  FetchByMetadata(song);
#endif
}
