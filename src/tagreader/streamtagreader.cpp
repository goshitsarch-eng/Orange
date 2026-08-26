#include "tagreader/streamtagreader.h"

#include "core/logging.h"
#include "core/network.h"

#include <algorithm>
#include <map>

const TagLibLengthType StreamTagReader::kPrefixCacheBytes = 64UL * 1024UL;
const TagLibLengthType StreamTagReader::kSuffixCacheBytes = 8UL * 1024UL;

StreamTagReader::StreamTagReader(const std::string &url, const std::string &filename, TagLibLengthType length, const std::string &token_type,
                                 const std::string &access_token, RangeFetcher fetcher)
    : url_(url),
      filename_(filename),
      length_(length),
      token_type_(token_type),
      access_token_(access_token),
      fetcher_(std::move(fetcher)) {
  if (!fetcher_ && !url_.empty()) {
    fetcher_ = HttpFetcher(url_, token_type_, access_token_);
  }
}

std::string StreamTagReader::RangeHeader(TagLibLengthType start, TagLibLengthType end) {
  return "bytes=" + std::to_string(start) + "-" + std::to_string(end);
}

std::string StreamTagReader::AuthorizationHeader(const std::string &token_type, const std::string &access_token) {
  if (token_type.empty() || access_token.empty()) {
    return {};
  }
  return token_type + " " + access_token;
}

StreamTagReader::RangeFetcher StreamTagReader::HttpFetcher(const std::string &url, const std::string &token_type, const std::string &access_token) {
  return [url, token_type, access_token](TagLibLengthType start, TagLibLengthType end) {
    NetworkAccessManager network;
    std::map<std::string, std::string> headers;
    headers["Range"] = RangeHeader(start, end);
    const std::string authorization = AuthorizationHeader(token_type, access_token);
    if (!authorization.empty()) {
      headers["Authorization"] = authorization;
    }
    const NetworkAccessManager::Response response = network.GetSync(url, headers);
    if (!response.ok()) {
      LogError("Unable to get tags from stream for %s: %s", url.c_str(),
               response.error.empty() ? std::to_string(response.status).c_str() : response.error.c_str());
      return std::string();
    }
    return response.body;
  };
}

TagLib::FileName StreamTagReader::name() const { return filename_.c_str(); }

TagLib::ByteVector StreamTagReader::readBlock(TagLibLengthType length) {
  if (length == 0 || cursor_ >= length_ || length_ == 0) {
    return TagLib::ByteVector();
  }
  const TagLibLengthType start = cursor_;
  const TagLibLengthType end = std::min(cursor_ + length - 1, length_ - 1);
  if (end < start) {
    return TagLib::ByteVector();
  }
  if (CheckCache(start, end)) {
    const TagLib::ByteVector cached = GetCache(start, end);
    cursor_ += static_cast<TagLibLengthType>(cached.size());
    return cached;
  }
  const std::string data = FetchRange(start, end);
  const TagLib::ByteVector bytes(data.data(), static_cast<unsigned>(data.size()));
  cursor_ += static_cast<TagLibLengthType>(data.size());
  FillCache(start, bytes);
  return bytes;
}

void StreamTagReader::writeBlock(const TagLib::ByteVector &) {}

void StreamTagReader::insert(const TagLib::ByteVector &, TagLibUOffsetType, TagLibLengthType) {}

void StreamTagReader::removeBlock(TagLibUOffsetType, TagLibLengthType) {}

bool StreamTagReader::readOnly() const { return true; }

bool StreamTagReader::isOpen() const { return true; }

void StreamTagReader::seek(TagLibOffsetType offset, TagLib::IOStream::Position position) {
  switch (position) {
    case TagLib::IOStream::Beginning:
      cursor_ = static_cast<TagLibLengthType>(offset < 0 ? 0 : offset);
      break;
    case TagLib::IOStream::Current:
      if (offset < 0) {
        const TagLibLengthType back = static_cast<TagLibLengthType>(-offset);
        cursor_ = back >= cursor_ ? 0 : cursor_ - back;
      } else {
        cursor_ = std::min(cursor_ + static_cast<TagLibLengthType>(offset), length_);
      }
      break;
    case TagLib::IOStream::End: {
      const TagLibLengthType abs_offset = offset < 0 ? static_cast<TagLibLengthType>(-offset) : static_cast<TagLibLengthType>(offset);
      cursor_ = abs_offset >= length_ ? 0 : length_ - abs_offset;
      break;
    }
  }
}

void StreamTagReader::clear() { cursor_ = 0; }

TagLibOffsetType StreamTagReader::tell() const { return static_cast<TagLibOffsetType>(cursor_); }

TagLibOffsetType StreamTagReader::length() { return static_cast<TagLibOffsetType>(length_); }

void StreamTagReader::truncate(TagLibOffsetType) {}

TagLibLengthType StreamTagReader::cached_bytes() const {
  TagLibLengthType total = 0;
  for (const Span &span : cache_) {
    total += static_cast<TagLibLengthType>(span.data.size());
  }
  return total;
}

void StreamTagReader::PreCache() {
  seek(0, TagLib::IOStream::Beginning);
  readBlock(kPrefixCacheBytes);
  seek(static_cast<TagLibOffsetType>(kSuffixCacheBytes), TagLib::IOStream::End);
  readBlock(kSuffixCacheBytes);
  clear();
}

bool StreamTagReader::CheckCache(TagLibLengthType start, TagLibLengthType end) const {
  TagLibLengthType pos = start;
  while (pos <= end) {
    bool found = false;
    for (const Span &span : cache_) {
      const TagLibLengthType span_end = span.start + static_cast<TagLibLengthType>(span.data.size());
      if (pos >= span.start && pos < span_end) {
        pos = span_end;
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

void StreamTagReader::FillCache(TagLibLengthType start, const TagLib::ByteVector &data) {
  if (data.size() == 0) {
    return;
  }
  Span span;
  span.start = start;
  span.data.assign(data.begin(), data.end());
  cache_.push_back(std::move(span));
}

TagLib::ByteVector StreamTagReader::GetCache(TagLibLengthType start, TagLibLengthType end) const {
  const TagLibLengthType size = end - start + 1;
  TagLib::ByteVector data(static_cast<unsigned>(size));
  for (TagLibLengthType i = 0; i < size; ++i) {
    const TagLibLengthType pos = start + i;
    bool found = false;
    for (const Span &span : cache_) {
      if (pos >= span.start && pos < span.start + static_cast<TagLibLengthType>(span.data.size())) {
        data[static_cast<unsigned>(i)] = span.data[pos - span.start];
        found = true;
        break;
      }
    }
    if (!found) {
      return TagLib::ByteVector();
    }
  }
  return data;
}

std::string StreamTagReader::FetchRange(TagLibLengthType start, TagLibLengthType end) {
  ++num_requests_;
  if (!fetcher_) {
    return {};
  }
  return fetcher_(start, end);
}
