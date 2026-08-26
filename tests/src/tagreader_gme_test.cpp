#include "tagreader/tagreadergme.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {

std::string MakeSpc(const std::string &title, const std::string &game, const std::string &artist, const char intro[3]) {
  std::string data(0xD1 + 32, '\0');
  const char header[] = "SNES-SPC700 Sound File Data v0.30";
  std::memcpy(&data[0], header, sizeof(header) - 1);
  std::memcpy(&data[TagReaderGME::kSongTitleOffset], title.data(), title.size());
  std::memcpy(&data[TagReaderGME::kGameTitleOffset], game.data(), game.size());
  std::memcpy(&data[TagReaderGME::kArtistOffset], artist.data(), artist.size());
  std::memcpy(&data[TagReaderGME::kIntroLengthOffset], intro, 3);
  return data;
}

void WriteLe32(std::string *data, size_t offset, uint32_t value) {
  (*data)[offset] = static_cast<char>(value & 0xFF);
  (*data)[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
  (*data)[offset + 2] = static_cast<char>((value >> 16) & 0xFF);
  (*data)[offset + 3] = static_cast<char>((value >> 24) & 0xFF);
}

void AppendUtf16(std::string *data, const std::string &text) {
  for (unsigned char ch : text) {
    data->push_back(static_cast<char>(ch));
    data->push_back('\0');
  }
  data->push_back('\0');
  data->push_back('\0');
}

std::string MakeVgm() {
  std::string header(0x40, '\0');
  header.replace(0, 4, "Vgm ");
  WriteLe32(&header, TagReaderGME::kGd3TagPtr, 0x40 - TagReaderGME::kGd3TagPtr);
  WriteLe32(&header, TagReaderGME::kSampleCount, 44100);
  WriteLe32(&header, TagReaderGME::kLoopSampleCount, 0);

  std::string gd3_payload;
  AppendUtf16(&gd3_payload, "Chip Tune");
  AppendUtf16(&gd3_payload, "");
  AppendUtf16(&gd3_payload, "Game OST");
  AppendUtf16(&gd3_payload, "");
  AppendUtf16(&gd3_payload, "");
  AppendUtf16(&gd3_payload, "");
  AppendUtf16(&gd3_payload, "Composer");
  AppendUtf16(&gd3_payload, "");
  AppendUtf16(&gd3_payload, "1994-05-01");
  AppendUtf16(&gd3_payload, "");

  std::string gd3(12, '\0');
  gd3.replace(0, 4, "Gd3 ");
  WriteLe32(&gd3, 4, 0x100);
  WriteLe32(&gd3, 8, static_cast<uint32_t>(gd3_payload.size()));
  return header + gd3 + gd3_payload;
}

}  // namespace

TEST(TagReaderGME, IsSupportedByExtension) {
  EXPECT_TRUE(TagReaderGME::IsSupported("song.spc"));
  EXPECT_TRUE(TagReaderGME::IsSupported("track.VGM"));
  EXPECT_FALSE(TagReaderGME::IsSupported("song.flac"));
}

TEST(TagReaderGME, ReadsSpcHeaderTags) {
  const char intro[] = {'1', '2', '0'};
  const std::string data = MakeSpc("Dummy Title", "Dummy Game", "Dummy Artist", intro);
  Song song;
  ASSERT_TRUE(TagReaderGME::ReadSpcData(data, &song));
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ(Song::FileType::SPC, song.filetype());
  EXPECT_EQ("Dummy Title", song.title());
  EXPECT_EQ("Dummy Game", song.album());
  EXPECT_EQ("Dummy Artist", song.artist());
  EXPECT_EQ(120 * 1000000000LL, song.length_nanosec());
}

TEST(TagReaderGME, RejectsInvalidSpcHeader) {
  Song song;
  EXPECT_FALSE(TagReaderGME::ReadSpcData("not an spc file", &song));
}

TEST(TagReaderGME, ReadsVgmGd3Tags) {
  const std::string data = MakeVgm();
  Song song;
  ASSERT_TRUE(TagReaderGME::ReadVgmData(data, &song));
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ(Song::FileType::VGM, song.filetype());
  EXPECT_EQ("Chip Tune", song.title());
  EXPECT_EQ("Game OST", song.album());
  EXPECT_EQ("Composer", song.artist());
  EXPECT_EQ(1994, song.year());
  EXPECT_EQ(1000000000LL, song.length_nanosec());
}

TEST(TagReaderGME, UnpackAndSpcNumber) {
  const char bytes[] = {0x44, static_cast<char>(0xAC), 0x00, 0x00};
  EXPECT_EQ(44100u, TagReaderGME::UnpackBytes32(bytes, 4));
  EXPECT_EQ(120u, TagReaderGME::ConvertSPCStringToNum("120", 3));
}

TEST(TagReaderGME, PlaybackLengthWithLoop) {
  const char samples[] = {static_cast<char>(0x88), 0x13, 0x00, 0x00};  // 5000
  const char loop[] = {static_cast<char>(0xE8), 0x03, 0x00, 0x00};     // 1000
  uint64_t length_ms = 0;
  ASSERT_TRUE(TagReaderGME::GetPlaybackLengthMs(samples, loop, &length_ms));
  const uint64_t intro_ms = (5000 - 1000) * 1000 / 44100;
  const uint64_t loop_ms = 1000 * 1000 / 44100;
  EXPECT_EQ(intro_ms + (loop_ms * 2) + TagReaderGME::kGstGmeLoopTimeMs, length_ms);
}
