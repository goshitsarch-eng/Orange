#ifndef STRAWBERRY_STREAMTAGREADER_H
#define STRAWBERRY_STREAMTAGREADER_H

#include <taglib/taglib.h>
#include <taglib/tiostream.h>

#include <functional>
#include <string>
#include <vector>

#if TAGLIB_MAJOR_VERSION >= 2
using TagLibLengthType = size_t;
using TagLibUOffsetType = TagLib::offset_t;
using TagLibOffsetType = TagLib::offset_t;
#else
using TagLibLengthType = unsigned long;
using TagLibUOffsetType = unsigned long;
using TagLibOffsetType = long;
#endif

class StreamTagReader : public TagLib::IOStream {
 public:
  using RangeFetcher = std::function<std::string(TagLibLengthType start, TagLibLengthType end)>;

  static const TagLibLengthType kPrefixCacheBytes;
  static const TagLibLengthType kSuffixCacheBytes;

  StreamTagReader(const std::string &url, const std::string &filename, TagLibLengthType length, const std::string &token_type = {},
                  const std::string &access_token = {}, RangeFetcher fetcher = {});

  TagLib::FileName name() const override;
  TagLib::ByteVector readBlock(TagLibLengthType length) override;
  void writeBlock(const TagLib::ByteVector &data) override;
  void insert(const TagLib::ByteVector &data, TagLibUOffsetType start, TagLibLengthType replace) override;
  void removeBlock(TagLibUOffsetType start, TagLibLengthType length) override;
  bool readOnly() const override;
  bool isOpen() const override;
  void seek(TagLibOffsetType offset, TagLib::IOStream::Position position) override;
  void clear() override;
  TagLibOffsetType tell() const override;
  TagLibOffsetType length() override;
  void truncate(TagLibOffsetType length) override;

  int num_requests() const { return num_requests_; }
  TagLibLengthType cached_bytes() const;
  void PreCache();

  static std::string RangeHeader(TagLibLengthType start, TagLibLengthType end);
  static std::string AuthorizationHeader(const std::string &token_type, const std::string &access_token);
  static RangeFetcher HttpFetcher(const std::string &url, const std::string &token_type, const std::string &access_token);

 private:
  bool CheckCache(TagLibLengthType start, TagLibLengthType end) const;
  void FillCache(TagLibLengthType start, const TagLib::ByteVector &data);
  TagLib::ByteVector GetCache(TagLibLengthType start, TagLibLengthType end) const;
  std::string FetchRange(TagLibLengthType start, TagLibLengthType end);

  std::string url_;
  std::string filename_;
  TagLibLengthType length_ = 0;
  std::string token_type_;
  std::string access_token_;
  RangeFetcher fetcher_;
  TagLibLengthType cursor_ = 0;
  int num_requests_ = 0;

  struct Span {
    TagLibLengthType start = 0;
    std::vector<char> data;
  };
  std::vector<Span> cache_;
};

#endif
