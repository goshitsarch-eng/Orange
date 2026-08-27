#include "device/mtploader.h"

#include "core/logging.h"
#include "device/mtpconnection.h"

#include <cstdlib>
#include <cstring>

std::string MtpLoader::ParseHost(const std::string &url) {
  if (url.rfind("mtp://", 0) != 0) {
    return {};
  }
  const std::string rest = url.substr(6);
  const auto slash = rest.find('/');
  return slash == std::string::npos ? rest : rest.substr(0, slash);
}

uint32_t MtpLoader::ParseItemId(const std::string &url) {
  if (url.rfind("mtp://", 0) != 0) {
    return 0;
  }
  const std::string rest = url.substr(6);
  const auto slash = rest.find('/');
  if (slash == std::string::npos) {
    return 0;
  }
  return static_cast<uint32_t>(std::strtoul(rest.substr(slash + 1).c_str(), nullptr, 10));
}

std::string MtpLoader::MakeUrl(const std::string &host, uint32_t item_id) {
  return "mtp://" + host + "/" + std::to_string(item_id);
}

#ifdef HAVE_MTP
Song MtpLoader::SongFromTrack(const LIBMTP_track_t *track, const std::string &host) {
  Song song(Song::Source::Device);
  if (!track) {
    return song;
  }
  song.set_valid(true);
  song.set_title(track->title ? track->title : "");
  song.set_artist(track->artist ? track->artist : "");
  song.set_album(track->album ? track->album : "");
  song.set_genre(track->genre ? track->genre : "");
  song.set_composer(track->composer ? track->composer : "");
  song.set_track(static_cast<int>(track->tracknumber));
  song.set_url(MakeUrl(host, track->item_id));
  song.set_basefilename(std::to_string(track->item_id));
  song.set_filesize(static_cast<int64_t>(track->filesize));
  song.set_mtime(track->modificationdate);
  song.set_ctime(track->modificationdate);
  song.set_length_nanosec(static_cast<int64_t>(track->duration) * 1000000LL);
  song.set_samplerate(static_cast<int>(track->samplerate));
  song.set_bitrate(static_cast<int>(track->bitrate));
  song.set_playcount(track->usecount);
  const Song::FileType type = MtpConnection::FileTypeFromMtp(track->filetype);
  song.set_filetype(type);
  if (type == Song::FileType::Unknown) {
    song.set_valid(false);
  }
  if (song.title().empty() && track->filename) {
    song.set_title(track->filename);
    song.set_valid(true);
  }
  return song;
}

void MtpLoader::SongToTrack(const Song &song, LIBMTP_track_t *track) {
  if (!track) {
    return;
  }
  track->item_id = 0;
  track->parent_id = 0;
  track->storage_id = 0;
  track->title = strdup(song.title().c_str());
  track->artist = strdup(song.EffectiveAlbumartist().c_str());
  track->album = strdup(song.album().c_str());
  track->genre = strdup(song.genre().c_str());
  track->date = nullptr;
  track->tracknumber = song.track() > 0 ? static_cast<uint16_t>(song.track()) : 0;
  track->composer = song.composer().empty() ? nullptr : strdup(song.composer().c_str());
  track->filename = strdup(song.basefilename().empty() ? song.title().c_str() : song.basefilename().c_str());
  track->filesize = static_cast<uint64_t>(song.filesize() > 0 ? song.filesize() : 0);
  track->modificationdate = static_cast<time_t>(song.mtime() > 0 ? song.mtime() : 0);
  track->duration = static_cast<uint32_t>(song.length_nanosec() / 1000000LL);
  track->bitrate = song.bitrate() > 0 ? static_cast<uint16_t>(song.bitrate()) : 0;
  track->bitratetype = 0;
  track->samplerate = song.samplerate() > 0 ? static_cast<uint32_t>(song.samplerate()) : 0;
  track->nochannels = 0;
  track->wavecodec = 0;
  track->usecount = song.playcount();
  track->filetype = MtpConnection::MtpFileTypeFromSong(song.filetype());
}
#endif

SongList MtpLoader::LoadSongs(const std::string &serial) {
  SongList songs;
#ifdef HAVE_MTP
  MtpConnection connection;
  if (!connection.OpenBySerial(serial)) {
    LogError("Could not open MTP device %s: %s", serial.c_str(), connection.error_text().c_str());
    return songs;
  }
  const std::string host = connection.serial().empty() ? serial : connection.serial();
  LIBMTP_track_t *tracks = LIBMTP_Get_Tracklisting_With_Callback(connection.device(), nullptr, nullptr);
  if (tracks) {
    while (tracks) {
      LIBMTP_track_t *track = tracks;
      tracks = tracks->next;
      Song song = SongFromTrack(track, host);
      if (song.is_valid() && !song.title().empty()) {
        songs.push_back(song);
      }
      LIBMTP_destroy_track_t(track);
    }
    return songs;
  }
  LIBMTP_file_t *file = LIBMTP_Get_Filelisting_With_Callback(connection.device(), nullptr, nullptr);
  while (file) {
    LIBMTP_file_t *next = file->next;
    if (LIBMTP_FILETYPE_IS_AUDIO(file->filetype) && file->filename) {
      Song song(Song::Source::Device);
      song.set_url(MakeUrl(host, file->item_id));
      song.set_title(file->filename);
      song.set_basefilename(file->filename);
      song.set_filesize(static_cast<int64_t>(file->filesize));
      song.set_valid(true);
      songs.push_back(song);
    }
    LIBMTP_destroy_file_t(file);
    file = next;
  }
#else
  (void)serial;
#endif
  return songs;
}
