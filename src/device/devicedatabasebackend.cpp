#include "device/devicedatabasebackend.h"

#include "core/logging.h"

const int DeviceDatabaseBackend::kDeviceSchemaVersion = 6;

DeviceDatabaseBackend::DeviceDatabaseBackend(Database *db) : db_(db) {}

bool DeviceDatabaseBackend::Init() { return EnsureTables(); }

bool DeviceDatabaseBackend::EnsureTables() {
  if (!db_ || !db_->handle()) {
    return false;
  }
  return db_->Exec(
      "CREATE TABLE IF NOT EXISTS devices ("
      "unique_id TEXT UNIQUE NOT NULL, "
      "friendly_name TEXT, "
      "size INTEGER, "
      "icon TEXT, "
      "schema_version INTEGER, "
      "transcode_mode INTEGER, "
      "transcode_format INTEGER)");
}

std::string DeviceDatabaseBackend::SongsTable(int device_id) const { return "device_" + std::to_string(device_id) + "_songs"; }

std::vector<DeviceDatabaseBackend::Device> DeviceDatabaseBackend::GetAllDevices() const {
  std::vector<Device> devices;
  if (!db_) {
    return devices;
  }
  SqlQuery query(db_,
                 "SELECT ROWID, unique_id, friendly_name, size, icon, schema_version, transcode_mode, transcode_format "
                 "FROM devices");
  while (query.Step()) {
    Device device;
    device.id = query.ColumnInt(0);
    device.unique_id = query.ColumnText(1);
    device.friendly_name = query.ColumnText(2);
    device.size = query.ColumnInt64(3);
    device.icon_name = query.ColumnText(4);
    device.schema_version = query.ColumnInt(5);
    device.transcode_mode = static_cast<TranscodeMode>(query.ColumnInt(6));
    device.transcode_format = static_cast<Song::FileType>(query.ColumnInt(7));
    if (device.schema_version >= kDeviceSchemaVersion) {
      devices.push_back(device);
    }
  }
  return devices;
}

int DeviceDatabaseBackend::AddDevice(const Device &device) {
  if (!db_) {
    return -1;
  }
  const Device existing = FindByUniqueId(device.unique_id);
  if (existing.id >= 0) {
    SetDeviceOptions(existing.id, device.friendly_name, device.icon_name, device.transcode_mode, device.transcode_format);
    return existing.id;
  }
  SqlQuery query(db_,
                 "INSERT INTO devices (unique_id, friendly_name, size, icon, schema_version, transcode_mode, transcode_format) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?)");
  query.Bind(1, device.unique_id);
  query.Bind(2, device.friendly_name);
  query.Bind(3, device.size);
  query.Bind(4, device.icon_name);
  query.Bind(5, kDeviceSchemaVersion);
  query.Bind(6, static_cast<int>(device.transcode_mode));
  query.Bind(7, static_cast<int>(device.transcode_format));
  if (!query.Exec()) {
    LogError("Could not add device %s: %s", device.unique_id.c_str(), db_->LastError().c_str());
    return -1;
  }
  const int id = static_cast<int>(db_->LastInsertRowId());
  const std::string table = SongsTable(id);
  db_->Exec("CREATE TABLE IF NOT EXISTS " + table +
            " (url TEXT, title TEXT, artist TEXT, album TEXT, albumartist TEXT, genre TEXT, composer TEXT, "
            "track INTEGER, year INTEGER, length INTEGER, bitrate INTEGER, samplerate INTEGER, filesize INTEGER, "
            "filename TEXT, playcount INTEGER, filetype INTEGER)");
  return id;
}

void DeviceDatabaseBackend::RemoveDevice(int id) {
  if (!db_ || id < 0) {
    return;
  }
  db_->Exec("DROP TABLE IF EXISTS " + SongsTable(id));
  SqlQuery query(db_, "DELETE FROM devices WHERE ROWID = ?");
  query.Bind(1, id);
  query.Exec();
}

void DeviceDatabaseBackend::SetDeviceOptions(int id, const std::string &friendly_name, const std::string &icon_name, TranscodeMode mode,
                                             Song::FileType format) {
  if (!db_ || id < 0) {
    return;
  }
  SqlQuery query(db_, "UPDATE devices SET friendly_name = ?, icon = ?, transcode_mode = ?, transcode_format = ? WHERE ROWID = ?");
  query.Bind(1, friendly_name);
  query.Bind(2, icon_name);
  query.Bind(3, static_cast<int>(mode));
  query.Bind(4, static_cast<int>(format));
  query.Bind(5, id);
  query.Exec();
}

DeviceDatabaseBackend::Device DeviceDatabaseBackend::FindByUniqueId(const std::string &unique_id) const {
  Device device;
  if (!db_) {
    return device;
  }
  SqlQuery query(db_,
                 "SELECT ROWID, unique_id, friendly_name, size, icon, schema_version, transcode_mode, transcode_format "
                 "FROM devices WHERE unique_id = ?");
  query.Bind(1, unique_id);
  if (query.Step()) {
    device.id = query.ColumnInt(0);
    device.unique_id = query.ColumnText(1);
    device.friendly_name = query.ColumnText(2);
    device.size = query.ColumnInt64(3);
    device.icon_name = query.ColumnText(4);
    device.schema_version = query.ColumnInt(5);
    device.transcode_mode = static_cast<TranscodeMode>(query.ColumnInt(6));
    device.transcode_format = static_cast<Song::FileType>(query.ColumnInt(7));
  }
  return device;
}

bool DeviceDatabaseBackend::ReplaceSongs(int device_id, const SongList &songs) {
  if (!db_ || device_id < 0) {
    return false;
  }
  const std::string table = SongsTable(device_id);
  db_->Exec("CREATE TABLE IF NOT EXISTS " + table +
            " (url TEXT, title TEXT, artist TEXT, album TEXT, albumartist TEXT, genre TEXT, composer TEXT, "
            "track INTEGER, year INTEGER, length INTEGER, bitrate INTEGER, samplerate INTEGER, filesize INTEGER, "
            "filename TEXT, playcount INTEGER, filetype INTEGER)");
  db_->Exec("DELETE FROM " + table);
  for (const Song &song : songs) {
    SqlQuery query(db_, "INSERT INTO " + table +
                            " (url, title, artist, album, albumartist, genre, composer, track, year, length, bitrate, "
                            "samplerate, filesize, filename, playcount, filetype) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.Bind(1, song.url());
    query.Bind(2, song.title());
    query.Bind(3, song.artist());
    query.Bind(4, song.album());
    query.Bind(5, song.albumartist());
    query.Bind(6, song.genre());
    query.Bind(7, song.composer());
    query.Bind(8, song.track());
    query.Bind(9, song.year());
    query.Bind(10, song.length_nanosec());
    query.Bind(11, song.bitrate());
    query.Bind(12, song.samplerate());
    query.Bind(13, song.filesize());
    query.Bind(14, song.basefilename());
    query.Bind(15, static_cast<int>(song.playcount()));
    query.Bind(16, static_cast<int>(song.filetype()));
    if (!query.Exec()) {
      return false;
    }
  }
  return true;
}

SongList DeviceDatabaseBackend::Songs(int device_id) const {
  SongList songs;
  if (!db_ || device_id < 0) {
    return songs;
  }
  SqlQuery query(db_, "SELECT url, title, artist, album, albumartist, genre, composer, track, year, length, bitrate, "
                      "samplerate, filesize, filename, playcount, filetype FROM " +
                          SongsTable(device_id));
  while (query.Step()) {
    Song song(Song::Source::Device);
    song.set_url(query.ColumnText(0));
    song.set_title(query.ColumnText(1));
    song.set_artist(query.ColumnText(2));
    song.set_album(query.ColumnText(3));
    song.set_albumartist(query.ColumnText(4));
    song.set_genre(query.ColumnText(5));
    song.set_composer(query.ColumnText(6));
    song.set_track(query.ColumnInt(7));
    song.set_year(query.ColumnInt(8));
    song.set_length_nanosec(query.ColumnInt64(9));
    song.set_bitrate(query.ColumnInt(10));
    song.set_samplerate(query.ColumnInt(11));
    song.set_filesize(query.ColumnInt64(12));
    song.set_basefilename(query.ColumnText(13));
    song.set_playcount(static_cast<unsigned>(query.ColumnInt(14)));
    song.set_filetype(static_cast<Song::FileType>(query.ColumnInt(15)));
    song.set_valid(true);
    songs.push_back(song);
  }
  return songs;
}
