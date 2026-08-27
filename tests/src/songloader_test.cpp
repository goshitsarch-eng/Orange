#include "core/songloader.h"
#include "core/songloadremote.h"
#include "core/urlhandlers.h"
#include "playlistparsers/playlistparser.h"
#include "utilities/fileutils.h"

#include <glib.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

namespace {

class FakeHandler : public UrlHandler {
 public:
  std::string scheme() const override { return "test"; }
  LoadResult Load(const std::string &url, AsyncCallback = {}) override {
    LoadResult result;
    result.type = LoadResult::Type::TrackAvailable;
    result.media_url = url;
    result.song.set_url(url);
    result.song.set_title("Handled");
    result.song.set_valid(true);
    return result;
  }
};

std::string TempDir() {
  char path[] = "/tmp/strawberry-songloader-XXXXXX";
  return mkdtemp(path);
}

}  // namespace

TEST(SongLoader, EmptyUrlIsError) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Error, loader.Load(""));
  EXPECT_FALSE(loader.errors().empty());
}

TEST(SongLoader, RawStreamSucceeds) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Success, loader.Load("https://example.com/stream.mp3"));
  ASSERT_EQ(1u, loader.songs().size());
  EXPECT_EQ("https://example.com/stream.mp3", loader.songs().front().url());
  EXPECT_TRUE(loader.songs().front().is_valid());
}

TEST(SongLoader, LoadManyStreams) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadMany({"https://a.example/x.mp3", "https://b.example/y.flac"}));
  EXPECT_EQ(2u, loader.songs().size());
}

TEST(SongLoader, RemotePolicyClassifiesProbeVsStream) {
  EXPECT_EQ(SongLoadRemote::Kind::RawStream, SongLoadRemote::Classify("rtsp://example.com/live"));
  EXPECT_EQ(SongLoadRemote::Kind::RawStream, SongLoadRemote::Classify("https://example.com/stream.mp3"));
  EXPECT_EQ(SongLoadRemote::Kind::RawStream, SongLoadRemote::Classify("https://example.com/stream.mp3?token=1"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://example.com/radio.m3u"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://example.com/radio.m3u8?id=2"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://example.com/list.pls"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://example.com/list.xspf"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://example.com/list.asx"));
  EXPECT_EQ(SongLoadRemote::Kind::Probe, SongLoadRemote::Classify("https://cdn.example/noext"));
  EXPECT_TRUE(SongLoadRemote::ShouldAddAsRawStream("https://example.com/stream.mp3"));
  EXPECT_FALSE(SongLoadRemote::ShouldAddAsRawStream("https://example.com/radio.m3u"));
}

TEST(SongLoader, RemotePlaylistRequiresBlockingLoad) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::BlockingLoadRequired, loader.Load("https://example.com/radio.m3u"));
  EXPECT_TRUE(loader.songs().empty());
  EXPECT_EQ(SongLoader::Result::BlockingLoadRequired, loader.Load("https://cdn.example/unknown"));
  EXPECT_EQ(SongLoader::Result::BlockingLoadRequired, loader.LoadMany({"https://a.example/x.mp3", "https://b.example/y.m3u"}));
}

TEST(SongLoader, LoadRemoteFromDataExpandsM3U) {
  SongLoader loader(nullptr, nullptr, nullptr);
  const std::string body =
      "#EXTM3U\n"
      "#EXTINF:180,Fleet Foxes - White Winter Hymnal\n"
      "https://example.com/white-winter-hymnal.mp3\n"
      "#EXTINF:240,Bon Iver - Holocene\n"
      "https://example.com/holocene.mp3\n";
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadRemoteFromData("https://example.com/radio.m3u", body));
  ASSERT_EQ(2u, loader.songs().size());
  EXPECT_EQ("https://example.com/white-winter-hymnal.mp3", loader.songs()[0].url());
  EXPECT_EQ("White Winter Hymnal", loader.songs()[0].title());
  EXPECT_EQ("Fleet Foxes", loader.songs()[0].artist());
  EXPECT_EQ("https://example.com/holocene.mp3", loader.songs()[1].url());
  EXPECT_EQ("radio.m3u", loader.playlist_name());
}

TEST(SongLoader, LoadRemoteFromDataExpandsXspfAndPls) {
  SongLoader loader(nullptr, nullptr, nullptr);
  const std::string xspf =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n"
      "  <trackList>\n"
      "    <track>\n"
      "      <location>https://example.com/a.mp3</location>\n"
      "      <title>Track A</title>\n"
      "      <creator>Artist A</creator>\n"
      "    </track>\n"
      "  </trackList>\n"
      "</playlist>\n";
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadRemoteFromData("https://example.com/list.xspf", xspf));
  ASSERT_EQ(1u, loader.songs().size());
  EXPECT_EQ("https://example.com/a.mp3", loader.songs().front().url());
  EXPECT_EQ("Track A", loader.songs().front().title());
  EXPECT_EQ("Artist A", loader.songs().front().artist());

  SongLoader pls_loader(nullptr, nullptr, nullptr);
  const std::string pls =
      "[playlist]\n"
      "NumberOfEntries=1\n"
      "File1=https://example.com/radio.mp3\n"
      "Title1=SomaFM Groove\n";
  EXPECT_EQ(SongLoader::Result::Success, pls_loader.LoadRemoteFromData("https://example.com/radio.pls", pls));
  ASSERT_EQ(1u, pls_loader.songs().size());
  EXPECT_EQ("https://example.com/radio.mp3", pls_loader.songs().front().url());
  EXPECT_EQ("SomaFM Groove", pls_loader.songs().front().title());
}

TEST(SongLoader, LoadRemoteFromDataUnknownBecomesRawStream) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadRemoteFromData("https://cdn.example/unknown", "ID3notaplaylist binary data"));
  ASSERT_EQ(1u, loader.songs().size());
  EXPECT_EQ("https://cdn.example/unknown", loader.songs().front().url());
  EXPECT_TRUE(loader.songs().front().is_valid());
}

TEST(SongLoader, LoadRemoteFromDataEmptyPlaylistIsError) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Error, loader.LoadRemoteFromData("https://example.com/empty.m3u", "#EXTM3U\n"));
  EXPECT_TRUE(loader.songs().empty());
  EXPECT_FALSE(loader.errors().empty());
}

TEST(SongLoader, PlaylistFile) {
  const std::string path = "/tmp/strawberry-songloader-" + std::to_string(getpid()) + ".m3u";
  Song song;
  song.set_title("White Winter Hymnal");
  song.set_artist("Fleet Foxes");
  song.set_url("file:///tmp/white-winter-hymnal.flac");
  song.set_valid(true);
  ASSERT_TRUE(PlaylistParser().Save(path, {song}));
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Success, loader.Load(path));
  ASSERT_FALSE(loader.songs().empty());
  EXPECT_FALSE(loader.songs().front().url().empty());
  unlink(path.c_str());
}

TEST(SongLoader, DirectoryRequiresBlockingLoad) {
  const std::string dir = TempDir();
  const std::string file = FileUtils::Join(dir, "track.flac");
  ASSERT_TRUE(FileUtils::WriteFile(file, "not-really-flac"));
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::BlockingLoadRequired, loader.Load(dir));
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadFilenamesBlocking());
  ASSERT_EQ(1u, loader.songs().size());
  EXPECT_TRUE(loader.songs().front().is_valid());
  FileUtils::Remove(file);
  rmdir(dir.c_str());
}

TEST(SongLoader, LoadFilenamesBlockingEmptyIsError) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Error, loader.LoadFilenamesBlocking());
}

TEST(SongLoader, LoadAudioCDIsError) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Error, loader.LoadAudioCD());
  EXPECT_FALSE(loader.errors().empty());
}

TEST(SongLoader, UsesUrlHandler) {
  FakeHandler handler;
  UrlHandlers handlers;
  handlers.AddHandler(&handler);
  SongLoader loader(&handlers, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Success, loader.Load("test://album/1"));
  ASSERT_EQ(1u, loader.songs().size());
  EXPECT_EQ("Handled", loader.songs().front().title());
}

TEST(SongLoader, MissingLocalFileIsError) {
  SongLoader loader(nullptr, nullptr, nullptr);
  EXPECT_EQ(SongLoader::Result::Error, loader.Load("file:///tmp/does-not-exist-strawberry-songloader.flac"));
  EXPECT_TRUE(loader.songs().empty());
  ASSERT_FALSE(loader.errors().empty());
  EXPECT_NE(std::string::npos, loader.errors().front().find("does not exist"));
  EXPECT_EQ(SongLoader::Result::Error, loader.Load("/tmp/does-not-exist-strawberry-songloader.flac"));
}
