#include "tagreader/tagreaderclient.h"
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

TEST(SaveTagsOptions, CombinesFlags) {
  const SaveTagsOptions all = SaveTagsOption::Tags | SaveTagsOption::Playcount | SaveTagsOption::Rating | SaveTagsOption::Cover;
  EXPECT_TRUE(HasSaveOption(all, SaveTagsOption::Tags));
  EXPECT_TRUE(HasSaveOption(all, SaveTagsOption::Cover));
  EXPECT_FALSE(HasSaveOption(static_cast<int>(SaveTagsOption::NoType), SaveTagsOption::Tags));
}
