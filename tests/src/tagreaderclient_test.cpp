#include "tagreader/albumcovertagdata.h"
#include "tagreader/tagreaderbase.h"
#include "tagreader/tagreaderclient.h"
#include "tagreader/tagreaderclientpump.h"
#include "playlist/playlist.h"
#include "playlist/playlistdelegates.h"
#include "playlist/playlistsaveitem.h"
#include "utilities/fileutils.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

namespace {

std::string Synchsafe(unsigned size) {
  std::string out(4, '\0');
  out[0] = static_cast<char>((size >> 21) & 0x7F);
  out[1] = static_cast<char>((size >> 14) & 0x7F);
  out[2] = static_cast<char>((size >> 7) & 0x7F);
  out[3] = static_cast<char>(size & 0x7F);
  return out;
}

std::string Be32(unsigned size) {
  std::string out(4, '\0');
  out[0] = static_cast<char>((size >> 24) & 0xFF);
  out[1] = static_cast<char>((size >> 16) & 0xFF);
  out[2] = static_cast<char>((size >> 8) & 0xFF);
  out[3] = static_cast<char>(size & 0xFF);
  return out;
}

std::string TextFrame(const char *id, const std::string &text) {
  const unsigned payload = 1 + static_cast<unsigned>(text.size());
  return std::string(id, 4) + Be32(payload) + std::string("\0\0", 2) + std::string("\x00", 1) + text;
}

std::string MakeId3Mpeg(const std::string &title, const std::string &artist, const std::string &album) {
  const std::string frames = TextFrame("TIT2", title) + TextFrame("TPE1", artist) + TextFrame("TALB", album);
  std::string data = "ID3";
  data.push_back(3);
  data.push_back(0);
  data.push_back(0);
  data += Synchsafe(static_cast<unsigned>(frames.size()));
  data += frames;
  data += std::string(256, '\0');
  data += "\xff\xfb\x90\x00";
  data += std::string(1024, '\0');
  return data;
}

std::string TempPath(const std::string &name) {
  return "/tmp/strawberry-tagreader-" + std::to_string(getpid()) + "-" + name;
}

}  // namespace

TEST(TagReaderResult, ErrorStringsMatchOriginalEnglish) {
  EXPECT_EQ("Success", TagReaderResult(TagReaderResult::ErrorCode::Success).error_string());
  EXPECT_EQ("File is unsupported", TagReaderResult(TagReaderResult::ErrorCode::Unsupported).error_string());
  EXPECT_EQ("Filename is missing", TagReaderResult(TagReaderResult::ErrorCode::FilenameMissing).error_string());
  EXPECT_EQ("File does not exist", TagReaderResult(TagReaderResult::ErrorCode::FileDoesNotExist).error_string());
  EXPECT_EQ("File could not be opened", TagReaderResult(TagReaderResult::ErrorCode::FileOpenError).error_string());
  EXPECT_EQ("Could not parse file", TagReaderResult(TagReaderResult::ErrorCode::FileParseError).error_string());
  EXPECT_EQ("Could not save file", TagReaderResult(TagReaderResult::ErrorCode::FileSaveError).error_string());
  EXPECT_EQ("custom", TagReaderResult(TagReaderResult::ErrorCode::CustomError, "custom").error_string());
}

TEST(TagReaderClient, PathResultEmptyMissingAndExisting) {
  TagReaderClient client;
  EXPECT_EQ(TagReaderResult::ErrorCode::FilenameMissing, client.PathResult({}).error_code);
  EXPECT_EQ(TagReaderResult::ErrorCode::FileDoesNotExist, client.PathResult("/tmp/does-not-exist-strawberry-tagreader").error_code);

  const std::string path = TempPath("exists.txt");
  FileUtils::WriteFile(path, "x");
  EXPECT_TRUE(client.PathResult(path).success());
  FileUtils::Remove(path);
}

TEST(TagReaderClient, IsMediaFileBlockingAndAsync) {
  TagReaderClient client;
  const std::string media = TempPath("track.mp3");
  const std::string text = TempPath("notes.txt");
  FileUtils::WriteFile(media, MakeId3Mpeg("Title", "Artist", "Album"));
  FileUtils::WriteFile(text, "not audio");

  EXPECT_TRUE(client.IsMediaFileBlocking(media));
  EXPECT_FALSE(client.IsMediaFileBlocking(text));
  EXPECT_FALSE(client.IsMediaFileBlocking({}));
  EXPECT_FALSE(client.IsMediaFileBlocking("/tmp/missing-strawberry-tagreader.mp3"));

  auto reply = client.IsMediaFileAsync(media);
  EXPECT_TRUE(client.HaveRequests());
  client.ProcessRequests();
  EXPECT_FALSE(client.HaveRequests());
  EXPECT_TRUE(reply->finished());
  EXPECT_TRUE(reply->success());

  auto missing = client.IsMediaFileAsync("/tmp/missing-strawberry-tagreader.mp3");
  client.ProcessRequests();
  EXPECT_EQ(TagReaderResult::ErrorCode::FileDoesNotExist, missing->result().error_code);

  FileUtils::Remove(media);
  FileUtils::Remove(text);
}

TEST(TagReaderClient, ReadFileAsyncEmitsSong) {
  TagReaderClient client;
  const std::string path = TempPath("read.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Helplessness Blues", "Fleet Foxes", "Helplessness Blues"));

  Song song;
  EXPECT_TRUE(client.ReadFileBlocking(path, &song).success());
  EXPECT_EQ("Helplessness Blues", song.title());
  EXPECT_EQ("Fleet Foxes", song.artist());

  Song received;
  bool finished = false;
  auto reply = client.ReadFileAsync(path);
  reply->SongFinished.Connect([&](const std::string &filename, const Song &read, const TagReaderResult &result) {
    finished = true;
    EXPECT_EQ(path, filename);
    EXPECT_TRUE(result.success());
    received = read;
  });
  client.ProcessRequests();
  EXPECT_TRUE(finished);
  EXPECT_EQ("Helplessness Blues", received.title());
  EXPECT_EQ("Fleet Foxes", received.artist());

  FileUtils::Remove(path);
}

TEST(TagReaderClient, WriteFileHonorsSaveOptionFlags) {
  TagReaderClient client;
  const std::string path = TempPath("write.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Original", "Artist", "Album"));

  Song song;
  ASSERT_TRUE(client.ReadFileBlocking(path, &song).success());
  song.set_title("Changed");
  song.set_playcount(12);
  song.set_rating(0.8f);

  SaveTagCoverData cover;
  cover.cover_data = {0xFF, 0xD8, 0xFF};
  cover.cover_mimetype = "image/jpeg";

  const SaveTagsOptions playcount_only = static_cast<int>(SaveTagsOption::Playcount);
  EXPECT_FALSE(HasSaveOption(playcount_only, SaveTagsOption::Tags));
  EXPECT_FALSE(HasSaveOption(playcount_only, SaveTagsOption::Cover));
  EXPECT_TRUE(HasSaveOption(playcount_only, SaveTagsOption::Playcount));

  const TagReaderResult result = client.WriteFileBlocking(path, song, playcount_only, cover);
  EXPECT_TRUE(result.success());

  Song reread;
  ASSERT_TRUE(client.ReadFileBlocking(path, &reread).success());
  EXPECT_EQ("Original", reread.title());

  std::vector<unsigned char> embedded;
  EXPECT_FALSE(client.LoadCoverDataBlocking(path, &embedded).success());
  EXPECT_TRUE(embedded.empty());

  FileUtils::Remove(path);
}

TEST(TagReaderClient, QueueAndClearAndBatchSave) {
  TagReaderClient client;
  EXPECT_FALSE(client.HaveRequests());

  auto reply = client.IsMediaFileAsync("/tmp/missing-strawberry-tagreader.mp3");
  EXPECT_TRUE(client.HaveRequests());
  client.Clear();
  EXPECT_FALSE(client.HaveRequests());
  EXPECT_FALSE(reply->finished());

  Song a;
  a.set_url(FileUtils::UriFromPath("/tmp/a.mp3"));
  a.set_playcount(3);
  a.set_rating(0.5f);
  Song b;
  b.set_url(FileUtils::UriFromPath("/tmp/b.mp3"));
  b.set_playcount(4);
  b.set_rating(1.0f);
  client.SaveSongsPlaycountAsync({a, b});
  client.SaveSongsRatingAsync({a, b});
  EXPECT_TRUE(client.HaveRequests());
  client.Clear();
  EXPECT_FALSE(client.HaveRequests());
}

TEST(TagReaderBase, ConvertPOPMRatingMatchesQt) {
  EXPECT_FLOAT_EQ(0.0f, TagReaderBase::ConvertPOPMRating(0));
  EXPECT_FLOAT_EQ(0.20f, TagReaderBase::ConvertPOPMRating(1));
  EXPECT_FLOAT_EQ(0.20f, TagReaderBase::ConvertPOPMRating(0x3F));
  EXPECT_FLOAT_EQ(0.40f, TagReaderBase::ConvertPOPMRating(0x40));
  EXPECT_FLOAT_EQ(0.60f, TagReaderBase::ConvertPOPMRating(0x80));
  EXPECT_FLOAT_EQ(0.80f, TagReaderBase::ConvertPOPMRating(0xC0));
  EXPECT_FLOAT_EQ(1.0f, TagReaderBase::ConvertPOPMRating(0xFC));
  EXPECT_FLOAT_EQ(1.0f, TagReaderBase::ConvertPOPMRating(0xFF));
  EXPECT_EQ(0x00, TagReaderBase::ConvertToPOPMRating(0.0f));
  EXPECT_EQ(0x01, TagReaderBase::ConvertToPOPMRating(0.20f));
  EXPECT_EQ(0x40, TagReaderBase::ConvertToPOPMRating(0.40f));
  EXPECT_EQ(0x80, TagReaderBase::ConvertToPOPMRating(0.60f));
  EXPECT_EQ(0xC0, TagReaderBase::ConvertToPOPMRating(0.80f));
  EXPECT_EQ(0xFF, TagReaderBase::ConvertToPOPMRating(1.0f));
}

TEST(AlbumCoverTagData, GuessMimeTypeFromMagic) {
  EXPECT_EQ("image/jpeg", AlbumCoverTagData::GuessMimeType({0xFF, 0xD8, 0xFF, 0xE0}));
  EXPECT_EQ("image/png", AlbumCoverTagData::GuessMimeType({0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}));
  EXPECT_EQ("image/gif", AlbumCoverTagData::GuessMimeType({'G', 'I', 'F', '8', '9', 'a'}));
}

TEST(TagReaderClient, SaveAndReadRatingAndCoverOnMpeg) {
  TagReaderClient client;
  const std::string path = TempPath("rated.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Rated", "Artist", "Album"));

  EXPECT_TRUE(client.SaveSongRatingBlocking(path, 0.8f).success());
  Song song;
  ASSERT_TRUE(client.ReadFileBlocking(path, &song).success());
  EXPECT_NEAR(0.8f, song.rating(), 0.001f);

  SaveTagCoverData cover;
  cover.cover_data = {0xFF, 0xD8, 0xFF, 0xE0};
  cover.cover_mimetype = "image/jpeg";
  EXPECT_TRUE(client.SaveCoverBlocking(path, cover).success());
  std::vector<unsigned char> embedded;
  EXPECT_TRUE(client.LoadCoverDataBlocking(path, &embedded).success());
  ASSERT_FALSE(embedded.empty());
  EXPECT_EQ(0xFF, embedded.front());

  Song reread;
  ASSERT_TRUE(client.ReadFileBlocking(path, &reread).success());
  EXPECT_TRUE(reread.art_embedded());

  FileUtils::Remove(path);
}

TEST(SaveTagsOptions, CombinesFlags) {
  const SaveTagsOptions all = SaveTagsOption::Tags | SaveTagsOption::Playcount | SaveTagsOption::Rating | SaveTagsOption::Cover;
  EXPECT_TRUE(HasSaveOption(all, SaveTagsOption::Tags));
  EXPECT_TRUE(HasSaveOption(all, SaveTagsOption::Cover));
  EXPECT_FALSE(HasSaveOption(static_cast<int>(SaveTagsOption::NoType), SaveTagsOption::Tags));
}

TEST(TagReader, WriteFileWritesId3v2ExtrasAndLyrics) {
  TagReader reader;
  const std::string path = TempPath("id3-extras.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Original", "Artist", "Album"));

  Song song = reader.ReadFile(path);
  song.set_title("Changed");
  song.set_albumartist("Album Artist");
  song.set_composer("Composer");
  song.set_lyrics("Line one\nLine two");
  song.set_disc(2);
  song.set_compilation(true);
  song.set_musicbrainz_recording_id("mbid-123");
  song.set_grouping("Group");
  ASSERT_TRUE(reader.WriteFile(path, song, static_cast<int>(SaveTagsOption::Tags)));

  Song reread = reader.ReadFile(path);
  EXPECT_EQ("Changed", reread.title());
  EXPECT_EQ("Album Artist", reread.albumartist());
  EXPECT_EQ("Composer", reread.composer());
  EXPECT_EQ("Line one\nLine two", reread.lyrics());
  EXPECT_EQ(2, reread.disc());
  EXPECT_TRUE(reread.compilation());
  EXPECT_EQ("mbid-123", reread.musicbrainz_recording_id());
  EXPECT_EQ("Group", reread.grouping());

  FileUtils::Remove(path);
}

TEST(TagReader, WriteFileCanSaveTagsRatingAndCoverTogether) {
  TagReader reader;
  const std::string path = TempPath("combined.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Title", "Artist", "Album"));

  Song song = reader.ReadFile(path);
  song.set_title("Together");
  song.set_rating(0.8f);
  song.set_playcount(9);
  TagReader::CoverData cover;
  cover.data = {0xFF, 0xD8, 0xFF, 0xE0};
  cover.mime_type = "image/jpeg";
  const SaveTagsOptions options =
      SaveTagsOption::Tags | SaveTagsOption::Rating | SaveTagsOption::Playcount | SaveTagsOption::Cover;
  ASSERT_TRUE(reader.WriteFile(path, song, options, cover));

  Song reread = reader.ReadFile(path);
  EXPECT_EQ("Together", reread.title());
  EXPECT_NEAR(0.8f, reread.rating(), 0.001f);
  EXPECT_GE(reread.playcount(), 9u);
  EXPECT_TRUE(reread.art_embedded());

  FileUtils::Remove(path);
}

TEST(TagReaderClient, ProcessNextDrainsOneRequest) {
  TagReaderClient client;
  EXPECT_FALSE(client.ProcessNext());
  const std::string path = TempPath("next.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Roads", "Portishead", "Dummy"));
  auto first = client.IsMediaFileAsync(path);
  auto second = client.IsMediaFileAsync(path);
  EXPECT_TRUE(client.HaveRequests());
  EXPECT_TRUE(TagReaderClientPump::ShouldArm(client.HaveRequests(), false));
  EXPECT_TRUE(client.ProcessNext());
  EXPECT_TRUE(first->finished());
  EXPECT_FALSE(second->finished());
  EXPECT_FALSE(client.ProcessNext());
  EXPECT_TRUE(second->finished());
  FileUtils::Remove(path);
}

TEST(Playlist, SaveRowsReloadsAfterAsyncWrite) {
  const std::string path = TempPath("save-row.mp3");
  FileUtils::WriteFile(path, MakeId3Mpeg("Roads", "Portishead", "Dummy"));
  TagReaderClient client;
  Playlist playlist;
  playlist.set_tagreader_client(&client);
  Song song(Song::Source::LocalFile);
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_url(FileUtils::UriFromPath(path));
  song.set_valid(true);
  playlist.AppendSongs({song});
  ASSERT_EQ(1, playlist.SetColumnValues({0}, PlaylistColumn::Title, "Glory Box"));
  playlist.SaveRows({0});
  EXPECT_TRUE(client.HaveRequests());
  client.ProcessRequests();
  EXPECT_EQ("Glory Box", playlist.song(0).title());
  const std::string uuid = playlist.UuidAt(0);
  const unsigned long long first = playlist.BumpSaveGeneration(uuid);
  const unsigned long long second = playlist.BumpSaveGeneration(uuid);
  EXPECT_FALSE(PlaylistSaveItem::ShouldApply(first, playlist.SaveGeneration(uuid)));
  EXPECT_TRUE(PlaylistSaveItem::ShouldApply(second, playlist.SaveGeneration(uuid)));
  FileUtils::Remove(path);
}
