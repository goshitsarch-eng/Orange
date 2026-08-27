#include "core/songloader.h"
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
  EXPECT_EQ(SongLoader::Result::Success, loader.LoadMany({"https://a.example/x", "https://b.example/y"}));
  EXPECT_EQ(2u, loader.songs().size());
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
