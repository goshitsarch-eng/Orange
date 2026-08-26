#include "tagfetcher/tagfetcher.h"

#include "config.h"
#include "core/logging.h"
#include "utilities/jsonutils.h"
#include "utilities/fileutils.h"

#ifdef HAVE_CHROMAPRINT
#include <chromaprint.h>
#endif

TagFetcher::TagFetcher(NetworkAccessManager *network) : network_(network) {}

void TagFetcher::Fetch(const Song &song) {
#ifdef HAVE_CHROMAPRINT
  LogInfo("Fingerprinting %s via AcoustID/MusicBrainz", song.url().c_str());
#endif
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
