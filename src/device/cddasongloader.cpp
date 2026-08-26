#include "device/cddasongloader.h"

#include "config.h"
#include "device/cddahelpers.h"
#include "device/devicemanager.h"

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif

SongList CddaSongLoader::Songs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec) {
  return DeviceManager::MakeCddaSongs(first_track, last_track, lengths_nanosec);
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
  for (track_t track = first; track <= last; ++track) {
    const lsn_t start = cdio_get_track_lsn(cdio, track);
    const lsn_t end = cdio_get_track_last_lsn(cdio, track);
    lengths.push_back(end > start ? static_cast<int64_t>(end - start) * 1000000000LL / 75 : 0);
  }
  cdio_destroy(cdio);
  return Songs(first, last, lengths);
#else
  (void)device_path;
  return {};
#endif
}
