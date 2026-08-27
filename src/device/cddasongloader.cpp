#include "device/cddasongloader.h"

#include "config.h"
#include "device/cddadiscid.h"
#include "device/cddahelpers.h"
#include "device/cddatext.h"
#include "device/devicemanager.h"

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#endif

SongList CddaSongLoader::Songs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec,
                               const std::string &device_path) {
  return DeviceManager::MakeCddaSongs(first_track, last_track, lengths_nanosec, device_path);
}

SongList CddaSongLoader::LoadDevice(const std::string &device_path) {
#ifdef HAVE_AUDIOCD
  if (CddaHelpers::ShouldSkipDevice(device_path)) {
    return {};
  }
  CddaHelpers::EnsureInit();
  CdIo_t *cdio = cdio_open(device_path.empty() ? nullptr : device_path.c_str(), DRIVER_DEVICE);
  if (!cdio) {
    return {};
  }
  const track_t first = cdio_get_first_track_num(cdio);
  const track_t last = cdio_get_last_track_num(cdio);
  if (!CddaHelpers::IsValidTrackRange(first, last)) {
    cdio_destroy(cdio);
    return {};
  }
  std::vector<int64_t> lengths;
  std::vector<int> offsets;
  for (track_t track = first; track <= last; ++track) {
    const lsn_t start = cdio_get_track_lsn(cdio, track);
    const lsn_t end = cdio_get_track_last_lsn(cdio, track);
    lengths.push_back(end > start ? static_cast<int64_t>(end - start) * 1000000000LL / 75 : 0);
    offsets.push_back(static_cast<int>(cdio_get_track_lba(cdio, track)));
  }
  const int leadout = static_cast<int>(cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK));
  const std::string disc_id = CddaDiscId::FromOffsets(first, last, leadout, offsets);

  std::string album;
  std::string album_artist;
  cdtext_t *cdtext = cdio_get_cdtext(cdio);
  if (cdtext) {
    if (const char *title = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, 0)) {
      album = title;
    }
    if (const char *performer = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, 0)) {
      album_artist = performer;
    }
  }

  SongList songs = Songs(first, last, lengths, device_path);
  for (Song &song : songs) {
    if (!disc_id.empty()) {
      song.set_musicbrainz_disc_id(disc_id);
    }
    std::string title;
    std::string artist;
    if (cdtext) {
      const track_t track = static_cast<track_t>(song.track());
      if (const char *value = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, track)) {
        title = value;
      }
      if (const char *value = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, track)) {
        artist = value;
      }
    }
    CddaText::Apply(&song, album, album_artist, title, artist);
  }
  cdio_destroy(cdio);
  return songs;
#else
  (void)device_path;
  return {};
#endif
}
