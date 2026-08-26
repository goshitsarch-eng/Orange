#include "device/gpodloader.h"

#include "core/logging.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#ifdef HAVE_GPOD

namespace {

std::string FromUtf8(const char *value) {
  return value ? value : "";
}

}  // namespace

Song GPodLoader::SongFromTrack(Itdb_Track *track, const std::string &prefix) {
  Song song(Song::Source::Device);
  if (!track) {
    return song;
  }
  song.set_valid(true);
  song.set_title(FromUtf8(track->title));
  song.set_album(FromUtf8(track->album));
  song.set_artist(FromUtf8(track->artist));
  song.set_albumartist(FromUtf8(track->albumartist));
  song.set_track(track->track_nr);
  song.set_disc(track->cd_nr);
  song.set_year(track->year);
  song.set_genre(FromUtf8(track->genre));
  song.set_compilation(track->compilation == 1);
  song.set_composer(FromUtf8(track->composer));
  song.set_grouping(FromUtf8(track->grouping));
  song.set_comment(FromUtf8(track->comment));
  song.set_length_nanosec(static_cast<int64_t>(track->tracklen) * 1000000LL);
  song.set_bitrate(track->bitrate);
  song.set_samplerate(track->samplerate);
  std::string filename = FromUtf8(track->ipod_path);
  filename = StrUtils::Replace(filename, ":", "/");
  if (!prefix.empty() && !filename.empty() && filename.front() != '/') {
    filename = "/" + filename;
  }
  const std::string path = prefix + filename;
  song.set_url(FileUtils::UriFromPath(path));
  song.set_basefilename(FileUtils::BaseName(filename));
  song.set_filetype(track->type2 ? Song::FileType::MPEG : Song::FileType::MP4);
  song.set_filesize(track->size);
  song.set_mtime(track->time_modified);
  song.set_ctime(track->time_added);
  song.set_playcount(track->playcount);
  song.set_skipcount(track->skipcount);
  song.set_lastplayed(track->time_played);
  return song;
}

void GPodLoader::SongToTrack(const Song &song, Itdb_Track *track) {
  if (!track) {
    return;
  }
  g_free(track->title);
  g_free(track->album);
  g_free(track->artist);
  g_free(track->albumartist);
  g_free(track->genre);
  g_free(track->composer);
  g_free(track->grouping);
  g_free(track->comment);
  track->title = g_strdup(song.title().c_str());
  track->album = g_strdup(song.album().c_str());
  track->artist = g_strdup(song.artist().c_str());
  track->albumartist = g_strdup(song.albumartist().c_str());
  track->track_nr = song.track();
  track->cd_nr = song.disc();
  track->year = song.year();
  track->genre = g_strdup(song.genre().c_str());
  track->compilation = song.compilation() ? 1 : 0;
  track->composer = g_strdup(song.composer().c_str());
  track->grouping = g_strdup(song.grouping().c_str());
  track->comment = g_strdup(song.comment().c_str());
  track->tracklen = static_cast<gint32>(song.length_nanosec() / 1000000LL);
  track->bitrate = song.bitrate();
  track->samplerate = song.samplerate();
  track->type1 = song.filetype() == Song::FileType::MPEG ? 1 : 0;
  track->type2 = song.filetype() == Song::FileType::MPEG ? 1 : 0;
  track->mediatype = 1;
  track->size = static_cast<guint32>(song.filesize() > 0 ? song.filesize() : 0);
  track->time_modified = static_cast<guint32>(song.mtime() > 0 ? song.mtime() : 0);
  track->time_added = static_cast<guint32>(song.ctime() > 0 ? song.ctime() : 0);
  track->playcount = song.playcount();
  track->skipcount = song.skipcount();
  track->time_played = static_cast<guint32>(song.lastplayed() > 0 ? song.lastplayed() : 0);
}

#endif  // HAVE_GPOD

SongList GPodLoader::LoadSongs(const std::string &mount_path) {
#ifdef HAVE_GPOD
  if (mount_path.empty()) {
    return {};
  }
  GError *error = nullptr;
  Itdb_iTunesDB *db = itdb_parse(mount_path.c_str(), &error);
  if (!db) {
    if (error) {
      LogWarning("Loading iPod database failed: %s", error->message);
      g_error_free(error);
    }
    return {};
  }
  SongList songs;
  for (GList *tracks = db->tracks; tracks; tracks = tracks->next) {
    songs.push_back(SongFromTrack(static_cast<Itdb_Track *>(tracks->data), mount_path));
  }
  itdb_free(db);
  return songs;
#else
  (void)mount_path;
  return {};
#endif
}
