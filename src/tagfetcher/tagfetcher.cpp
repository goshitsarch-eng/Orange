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
  acoustid_->Finished.Connect([this](int id, const std::vector<std::string> &mbids, const std::string &error) {
    if (cancelled_ || current_.id != id) {
      return;
    }
    if (!error.empty() || mbids.empty()) {
      FetchByMetadata(current_);
      return;
    }
    Progress.Emit(id, "Looking up MusicBrainz recording…");
    musicbrainz_->Start(id, mbids);
  });
  musicbrainz_->Finished.Connect([this](int id, const MusicBrainzClient::ResultList &results, const std::string &error) {
    if (cancelled_ || current_.id != id) {
      return;
    }
    if (!error.empty() && results.empty()) {
      FetchByMetadata(current_);
      return;
    }
    SongList songs = MusicBrainzClient::ToSongs(results);
    for (Song &song : songs) {
      song.set_url(current_.song.url());
      song.set_id(current_.song.id());
    }
    Complete(id, songs, {});
  });
}

int TagFetcher::Enqueue(const Song &song) {
  Job job;
  job.id = next_id_++;
  job.song = song;
  queue_.push_back(job);
  return job.id;
}

void TagFetcher::StartNext() {
  if (running_ || queue_.empty() || cancelled_) {
    return;
  }
  current_ = queue_.front();
  queue_.erase(queue_.begin());
  running_ = true;
  Progress.Emit(current_.id, "Fetching tags…");
#ifdef HAVE_CHROMAPRINT
  LogInfo("Fingerprinting %s via AcoustID/MusicBrainz", current_.song.url().c_str());
  FetchByFingerprint(current_);
#else
  FetchByMetadata(current_);
#endif
}

void TagFetcher::Complete(int id, SongList results, const std::string &error) {
  if (cancelled_) {
    return;
  }
  if (!error.empty()) {
    Error.Emit(id, error);
  }
  Results.Emit(results);
  SongResults.Emit(id, results);
  running_ = false;
  current_ = Job();
  if (queue_.empty()) {
    Finished.Emit();
    return;
  }
  StartNext();
}

void TagFetcher::FetchByMetadata(const Job &job) {
  if (!network_) {
    Complete(job.id, {}, "No network");
    return;
  }
  std::string query = job.song.artist() + " " + job.song.title();
  if (query.empty()) {
    query = FileUtils::BaseName(FileUtils::PathFromUri(job.song.url()));
  }
  if (query.empty()) {
    Complete(job.id, {}, "No metadata to search");
    return;
  }
  Progress.Emit(job.id, "Searching MusicBrainz…");
  gchar *escaped = g_uri_escape_string(query.c_str(), nullptr, TRUE);
  const std::string url = std::string("https://musicbrainz.org/ws/2/recording/?query=") + (escaped ? escaped : "") + "&fmt=json";
  g_free(escaped);
  const int id = job.id;
  const Song original = job.song;
  network_->Get(url, [this, id, original](const NetworkAccessManager::Response &response) {
    if (cancelled_ || current_.id != id) {
      return;
    }
    SongList results;
    std::string error;
    if (response.ok()) {
      results = JsonUtils::ParseMusicBrainzRecordings(response.body);
      for (Song &result : results) {
        result.set_url(original.url());
        result.set_id(original.id());
      }
    } else {
      error = response.error.empty() ? "MusicBrainz request failed" : response.error;
    }
    Complete(id, results, error);
  }, {{"User-Agent", "Strawberry/1.0 (https://www.strawberrymusicplayer.org/)"}});
}

void TagFetcher::FetchByFingerprint(const Job &job) {
#ifdef HAVE_CHROMAPRINT
  Progress.Emit(job.id, "Fingerprinting…");
  Chromaprinter printer(job.song.url());
  const std::string fingerprint = printer.CreateFingerprint();
  if (fingerprint.empty()) {
    FetchByMetadata(job);
    return;
  }
  const int duration_msec = job.song.length_nanosec() > 0 ? static_cast<int>(job.song.length_nanosec() / 1000000) : 30000;
  acoustid_->Start(job.id, fingerprint, duration_msec);
#else
  FetchByMetadata(job);
#endif
}

int TagFetcher::Fetch(const Song &song) {
  cancelled_ = false;
  const int id = Enqueue(song);
  StartNext();
  return id;
}

std::vector<int> TagFetcher::QueueSongs(const SongList &songs) {
  cancelled_ = false;
  std::vector<int> ids;
  ids.reserve(songs.size());
  for (const Song &song : songs) {
    ids.push_back(Enqueue(song));
  }
  return ids;
}

std::vector<int> TagFetcher::FetchSongs(const SongList &songs) {
  const std::vector<int> ids = QueueSongs(songs);
  StartNext();
  return ids;
}

void TagFetcher::Start() { StartNext(); }

void TagFetcher::Cancel() {
  cancelled_ = true;
  queue_.clear();
  running_ = false;
  current_ = Job();
  acoustid_->CancelAll();
  musicbrainz_->CancelAll();
  Finished.Emit();
}
