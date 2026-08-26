#include "collection/collectionsettingsdirectorymodel.h"
#include "constants/timeconstants.h"
#include "core/appearance.h"
#include "core/filewriteguard.h"
#include "core/filesystemwatcherqt.h"
#include "core/mimedata.h"
#include "core/sqlrow.h"
#include "core/threadsafenetworkdiskcache.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcovermanagerlist.h"
#include "covermanager/coveroptions.h"
#include "device/devicestatefiltermodel.h"
#include "engine/enginebase.h"
#include "radios/radiomimedata.h"
#include "utilities/fileutils.h"
#include "utilities/textencodingutils.h"
#include "utilities/transliterate.h"
#include "utilities/xmlparser.h"

#include <optional>

#include <gtest/gtest.h>

TEST(SqlRow, StoresColumns) {
  SqlRow row;
  row.Add("one");
  row.Add("two");
  EXPECT_EQ(2, row.columns());
  EXPECT_EQ("two", row.value(1));
}

TEST(MimeData, JoinsSongUrls) {
  MimeData data;
  Song song;
  song.set_url("file:///tmp/a.flac");
  data.songs.push_back(song);
  EXPECT_EQ("file:///tmp/a.flac", data.text());
}

TEST(TimeConstants, NanosecondScale) {
  EXPECT_EQ(1000000000, TimeConstants::kNsecPerSec);
  EXPECT_EQ(1000, TimeConstants::kMsecPerSec);
}

TEST(AlbumCoverExport, DialogResultDefaults) {
  AlbumCoverExport::DialogResult result;
  EXPECT_FALSE(result.IsSizeForced());
  EXPECT_FALSE(result.RequiresCoverProcessing());
  result.forcesize = true;
  result.width = 300;
  result.height = 300;
  EXPECT_TRUE(result.IsSizeForced());
}

TEST(AlbumCoverManagerList, GroupsAlbums) {
  Song a;
  a.set_album("Fleet Foxes");
  a.set_albumartist("Fleet Foxes");
  a.set_art_manual("/tmp/cover.jpg");
  Song b = a;
  b.set_title("White Winter Hymnal");
  Song c;
  c.set_album("Helplessness Blues");
  c.set_albumartist("Fleet Foxes");
  AlbumCoverManagerList list;
  list.SetSongs({a, b, c});
  EXPECT_EQ(2, list.album_count());
  EXPECT_EQ(1, list.with_cover_count());
}

TEST(DeviceStateFilterModel, FiltersConnected) {
  ConnectedDevice mounted;
  mounted.mount_path = "/media/usb";
  ConnectedDevice loose;
  DeviceStateFilterModel model;
  model.SetDevices({mounted, loose});
  model.set_state(DeviceStateFilterModel::State::Connected);
  EXPECT_EQ(1u, model.Filtered().size());
}

TEST(ThreadSafeNetworkDiskCache, InsertAndRead) {
  ThreadSafeNetworkDiskCache cache;
  cache.Insert("k", "v");
  EXPECT_EQ("v", cache.Value("k"));
  cache.Clear();
  EXPECT_TRUE(cache.Value("k").empty());
}

TEST(TextEncodingUtils, DetectsUtf8) {
  EXPECT_TRUE(TextEncodingUtils::IsUtf8("café"));
  EXPECT_EQ("plain", TextEncodingUtils::ToUtf8("plain"));
}

TEST(XmlParser, ReadsChildText) {
  EXPECT_EQ("Fleet Foxes", XmlParser::ChildText("<artist>Fleet Foxes</artist>", "artist"));
}

TEST(CollectionSettingsDirectoryModel, AddAndRemove) {
  CollectionSettingsDirectoryModel model;
  model.Add("/music");
  model.Add("/music");
  EXPECT_EQ(1u, model.paths().size());
  model.Remove("/music");
  EXPECT_TRUE(model.paths().empty());
}

TEST(FileWriteGuard, ChecksPath) {
  FileWriteGuard missing("/tmp/strawberry-does-not-exist-filewriteguard");
  EXPECT_FALSE(missing.ok());
  EXPECT_FALSE(missing.active());
  EXPECT_FALSE(FileUtils::FilenameOnGVFS("/home/user/music/track.mp3"));
  EXPECT_TRUE(FileUtils::FilenameOnGVFS("/run/user/1000/gvfs/smb-share:server=host/track.mp3"));
  EXPECT_TRUE(FileUtils::FilenameOnGVFS("/home/user/.gvfs/sftp/track.mp3"));
}

TEST(FileSystemWatcherQt, AddAndClear) {
  FileSystemWatcherQt watcher;
  watcher.AddPath("/tmp");
  watcher.RemovePath("/tmp");
  watcher.Clear();
}

TEST(RadioMimeData, FormatsChannels) {
  RadioMimeData data;
  data.channels.push_back({"SomaFM", "https://somafm.com", {}, {}, {}, {}, Song::Source::SomaFM});
  EXPECT_NE(std::string::npos, data.format().find("SomaFM"));
}

TEST(Appearance, ReloadsWithoutApply) {
  Appearance appearance;
  appearance.ReloadSettings();
  EXPECT_GE(appearance.opacity(), 0);
}

TEST(CoverOptions, Defaults) {
  CoverOptions options;
  EXPECT_EQ(300, options.desired_height);
  EXPECT_TRUE(options.scale);
}

TEST(EngineBase, DummyImplementation) {
  class DummyEngine : public EngineBase {
   public:
    bool Init() override { return true; }
    State state() const override { return State::Idle; }
    bool Load(const std::string &, const std::string &, int, bool, uint64_t, int64_t, std::optional<double>) override { return true; }
    bool Play(bool, uint64_t) override { return true; }
    void Stop(bool) override {}
    void Pause() override {}
    void Unpause() override {}
    void Seek(uint64_t) override {}
    void SetVolumeSW(unsigned) override {}
    int64_t position_nanosec() const override { return 0; }
    int64_t length_nanosec() const override { return 0; }
  };
  DummyEngine engine;
  EngineBase *base = &engine;
  EXPECT_TRUE(base->Init());
  EXPECT_EQ(EngineBase::State::Idle, base->state());
}

TEST(Transliterate, DelegatesToStrUtils) {
  EXPECT_FALSE(Transliterate("Ångström").empty());
}
