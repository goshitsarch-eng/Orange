#include "tagreader/streamtagreader.h"
#include "tagreader/tagreader.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

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

StreamTagReader::RangeFetcher BufferFetcher(const std::string &body, int *requests) {
  return [body, requests](TagLibLengthType start, TagLibLengthType end) {
    if (requests) {
      ++(*requests);
    }
    if (start >= body.size()) {
      return std::string();
    }
    const size_t last = std::min(static_cast<size_t>(end) + 1, body.size());
    return body.substr(static_cast<size_t>(start), last - static_cast<size_t>(start));
  };
}

}  // namespace

TEST(StreamTagReader, RangeAndAuthorizationHeaders) {
  EXPECT_EQ("bytes=0-1023", StreamTagReader::RangeHeader(0, 1023));
  EXPECT_EQ("bytes=65536-73727", StreamTagReader::RangeHeader(65536, 73727));
  EXPECT_EQ("Bearer tok", StreamTagReader::AuthorizationHeader("Bearer", "tok"));
  EXPECT_TRUE(StreamTagReader::AuthorizationHeader("Bearer", {}).empty());
  EXPECT_TRUE(StreamTagReader::AuthorizationHeader({}, "tok").empty());
}

TEST(StreamTagReader, SeekTellAndEndOffset) {
  const std::string body(100, 'x');
  StreamTagReader stream("https://example/stream.mp3", "stream.mp3", body.size(), {}, {}, BufferFetcher(body, nullptr));
  EXPECT_EQ(100, stream.length());
  EXPECT_EQ(0, stream.tell());
  stream.seek(10, TagLib::IOStream::Beginning);
  EXPECT_EQ(10, stream.tell());
  stream.seek(5, TagLib::IOStream::Current);
  EXPECT_EQ(15, stream.tell());
  stream.seek(8, TagLib::IOStream::End);
  EXPECT_EQ(92, stream.tell());
  stream.seek(-8, TagLib::IOStream::End);
  EXPECT_EQ(92, stream.tell());
  stream.seek(200, TagLib::IOStream::End);
  EXPECT_EQ(0, stream.tell());
  stream.clear();
  EXPECT_EQ(0, stream.tell());
  EXPECT_TRUE(stream.readOnly());
  EXPECT_TRUE(stream.isOpen());
}

TEST(StreamTagReader, CachesPrefixAndSuffixWithTwoRequests) {
  const std::string body(2000, 'a');
  StreamTagReader stream("https://example/stream.mp3", "stream.mp3", body.size(), {}, {}, BufferFetcher(body, nullptr));
  stream.PreCache();
  EXPECT_EQ(2, stream.num_requests());
  EXPECT_EQ(0, stream.tell());
  EXPECT_GT(stream.cached_bytes(), 0u);
  const TagLib::ByteVector first = stream.readBlock(16);
  EXPECT_EQ(16u, first.size());
  EXPECT_EQ(2, stream.num_requests());
}

TEST(StreamTagReader, ReadBlockUsesFetcherThenCache) {
  const std::string body = "abcdefghijklmnopqrstuvwxyz";
  StreamTagReader stream("https://example/stream.mp3", "stream.mp3", body.size(), {}, {}, BufferFetcher(body, nullptr));
  const TagLib::ByteVector first = stream.readBlock(4);
  EXPECT_EQ(std::string(first.data(), first.size()), "abcd");
  EXPECT_EQ(1, stream.num_requests());
  stream.seek(0, TagLib::IOStream::Beginning);
  const TagLib::ByteVector again = stream.readBlock(4);
  EXPECT_EQ(std::string(again.data(), again.size()), "abcd");
  EXPECT_EQ(1, stream.num_requests());
}

TEST(TagReader, ReadStreamParsesId3FromRangeFetcher) {
  const std::string body = MakeId3Mpeg("Roads", "Portishead", "Dummy");
  StreamTagReader stream("https://example/roads.mp3", "roads.mp3", body.size(), "Bearer", "tok", BufferFetcher(body, nullptr));
  TagReader reader;
  const Song song = reader.ReadStream(&stream, "https://example/roads.mp3", "roads.mp3", body.size(), 123456);
  EXPECT_EQ("https://example/roads.mp3", song.url());
  EXPECT_EQ("roads.mp3", song.basefilename());
  EXPECT_EQ(123456, song.mtime());
  EXPECT_EQ(Song::Source::Stream, song.source());
  EXPECT_EQ("Roads", song.title());
  EXPECT_EQ("Portishead", song.artist());
  EXPECT_EQ("Dummy", song.album());
  EXPECT_TRUE(song.is_valid());
}
