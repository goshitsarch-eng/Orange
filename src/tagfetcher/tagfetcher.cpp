#include "tagfetcher/tagfetcher.h"
#include "config.h"
#include "core/logging.h"
#ifdef HAVE_CHROMAPRINT
#include <chromaprint.h>
#endif
TagFetcher::TagFetcher(NetworkAccessManager *network) : network_(network) {}
void TagFetcher::Fetch(const Song &song) {
#ifdef HAVE_CHROMAPRINT
  LogInfo("Fingerprinting %s via AcoustID/MusicBrainz", song.url().c_str());
#endif
  if (!network_) { Results.Emit({}); return; }
  gchar *escaped = g_uri_escape_string((song.artist() + " " + song.title()).c_str(), nullptr, TRUE);
  const std::string url = std::string("https://musicbrainz.org/ws/2/recording/?query=") + (escaped ? escaped : "") + "&fmt=json";
  g_free(escaped);
  network_->Get(url, [this, song](const NetworkAccessManager::Response &response) {
    SongList results;
    if (response.ok()) results.push_back(song);
    Results.Emit(results);
  });
}
