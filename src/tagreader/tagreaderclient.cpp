#include "tagreader/tagreaderclient.h"

#include "utilities/fileutils.h"

#include <utility>

namespace {

TagReader::CoverData ToCoverData(const SaveTagCoverData &cover) {
  TagReader::CoverData data;
  data.data = cover.cover_data;
  data.mime_type = cover.cover_mimetype.empty() ? "image/jpeg" : cover.cover_mimetype;
  if (data.data.empty() && !cover.cover_filename.empty() && FileUtils::Exists(cover.cover_filename)) {
    const std::string bytes = FileUtils::ReadFile(cover.cover_filename);
    data.data.assign(bytes.begin(), bytes.end());
  }
  return data;
}

}  // namespace

TagReaderClient::TagReaderClient(TagReader *tagreader) : tagreader_(tagreader ? tagreader : &owned_) {}

bool TagReaderClient::HaveRequests() const { return !requests_.empty(); }

void TagReaderClient::EnqueueRequest(TagReaderRequestPtr request) { requests_.push(std::move(request)); }

TagReaderRequestPtr TagReaderClient::DequeueRequest() {
  if (requests_.empty()) {
    return {};
  }
  TagReaderRequestPtr request = requests_.front();
  requests_.pop();
  return request;
}

void TagReaderClient::Clear() {
  while (!requests_.empty()) {
    requests_.pop();
  }
}

void TagReaderClient::ProcessRequests() {
  while (HaveRequests()) {
    ProcessRequest(DequeueRequest());
  }
}

TagReaderResult TagReaderClient::PathResult(const std::string &filename) const {
  if (filename.empty()) {
    return {TagReaderResult::ErrorCode::FilenameMissing};
  }
  if (!FileUtils::Exists(filename)) {
    return {TagReaderResult::ErrorCode::FileDoesNotExist};
  }
  return {TagReaderResult::ErrorCode::Success};
}

bool TagReaderClient::IsMediaFileBlocking(const std::string &filename) const {
  return PathResult(filename).success() && tagreader_->IsMediaFile(filename);
}

TagReaderReplyPtr TagReaderClient::IsMediaFileAsync(const std::string &filename) {
  auto reply = std::make_shared<TagReaderReply>(filename);
  auto request = TagReaderIsMediaFileRequest::Create(filename);
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::ReadFileBlocking(const std::string &filename, Song *song) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  if (!tagreader_->IsMediaFile(filename)) {
    return {TagReaderResult::ErrorCode::Unsupported};
  }
  Song read = tagreader_->ReadFile(filename);
  if (song) {
    *song = read;
  }
  return {TagReaderResult::ErrorCode::Success};
}

TagReaderReadFileReplyPtr TagReaderClient::ReadFileAsync(const std::string &filename) {
  auto reply = std::make_shared<TagReaderReadFileReply>(filename);
  auto request = TagReaderReadFileRequest::Create(filename);
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::ReadStreamBlocking(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime,
                                                    const std::string &token_type, const std::string &access_token, Song *song) {
  if (url.empty()) {
    return {TagReaderResult::ErrorCode::FilenameMissing};
  }
  Song read = tagreader_->ReadStream(url, filename, size, mtime, token_type, access_token);
  if (song) {
    *song = read;
  }
  return read.is_valid() ? TagReaderResult{TagReaderResult::ErrorCode::Success} : TagReaderResult{TagReaderResult::ErrorCode::FileParseError};
}

TagReaderReadStreamReplyPtr TagReaderClient::ReadStreamAsync(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime,
                                                             const std::string &token_type, const std::string &access_token) {
  auto reply = std::make_shared<TagReaderReadStreamReply>(url, filename);
  auto request = TagReaderReadStreamRequest::Create(url, filename);
  request->size = size;
  request->mtime = mtime;
  request->token_type = token_type;
  request->access_token = access_token;
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::WriteFileBlocking(const std::string &filename, const Song &song, SaveTagsOptions save_tags_options,
                                                   const SaveTagCoverData &save_tag_cover_data, TagID3v2Version) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  bool ok = true;
  if (HasSaveOption(save_tags_options, SaveTagsOption::Tags)) {
    Song written = song;
    if (written.url().empty()) {
      written.set_url(FileUtils::UriFromPath(filename));
    }
    ok = tagreader_->WriteFile(written) && ok;
  }
  if (HasSaveOption(save_tags_options, SaveTagsOption::Playcount)) {
    ok = tagreader_->SavePlaycount(filename, song.playcount()) && ok;
  }
  if (HasSaveOption(save_tags_options, SaveTagsOption::Rating)) {
    ok = tagreader_->SaveRating(filename, song.rating()) && ok;
  }
  if (HasSaveOption(save_tags_options, SaveTagsOption::Cover)) {
    ok = tagreader_->SaveCover(filename, ToCoverData(save_tag_cover_data)) && ok;
  }
  return ok ? TagReaderResult{TagReaderResult::ErrorCode::Success} : TagReaderResult{TagReaderResult::ErrorCode::FileSaveError};
}

TagReaderReplyPtr TagReaderClient::WriteFileAsync(const std::string &filename, const Song &song, SaveTagsOptions save_tags_options,
                                                  const SaveTagCoverData &save_tag_cover_data, TagID3v2Version tag_id3v2_version) {
  auto reply = std::make_shared<TagReaderReply>(filename);
  auto request = TagReaderWriteFileRequest::Create(filename);
  request->song = song;
  request->save_tags_options = save_tags_options;
  request->save_tag_cover_data = save_tag_cover_data;
  request->tag_id3v2_version = tag_id3v2_version;
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::LoadCoverDataBlocking(const std::string &filename, std::vector<unsigned char> *data) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  const TagReader::CoverData cover = tagreader_->LoadCoverData(filename);
  if (data) {
    *data = cover.data;
  }
  return cover.data.empty() ? TagReaderResult{TagReaderResult::ErrorCode::FileParseError} : TagReaderResult{TagReaderResult::ErrorCode::Success};
}

TagReaderLoadCoverDataReplyPtr TagReaderClient::LoadCoverDataAsync(const std::string &filename) {
  auto reply = std::make_shared<TagReaderLoadCoverDataReply>(filename);
  auto request = TagReaderLoadCoverDataRequest::Create(filename);
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderLoadCoverImageReplyPtr TagReaderClient::LoadCoverImageAsync(const std::string &filename) { return LoadCoverDataAsync(filename); }

TagReaderResult TagReaderClient::SaveCoverBlocking(const std::string &filename, const SaveTagCoverData &save_tag_cover_data) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  return tagreader_->SaveCover(filename, ToCoverData(save_tag_cover_data)) ? TagReaderResult{TagReaderResult::ErrorCode::Success}
                                                                           : TagReaderResult{TagReaderResult::ErrorCode::FileSaveError};
}

TagReaderReplyPtr TagReaderClient::SaveCoverAsync(const std::string &filename, const SaveTagCoverData &save_tag_cover_data) {
  auto reply = std::make_shared<TagReaderReply>(filename);
  auto request = TagReaderSaveCoverRequest::Create(filename);
  request->save_tag_cover_data = save_tag_cover_data;
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::SaveSongPlaycountBlocking(const std::string &filename, unsigned playcount) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  return tagreader_->SavePlaycount(filename, playcount) ? TagReaderResult{TagReaderResult::ErrorCode::Success}
                                                        : TagReaderResult{TagReaderResult::ErrorCode::FileSaveError};
}

TagReaderReplyPtr TagReaderClient::SaveSongPlaycountAsync(const std::string &filename, unsigned playcount) {
  auto reply = std::make_shared<TagReaderReply>(filename);
  auto request = TagReaderSavePlaycountRequest::Create(filename);
  request->playcount = playcount;
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

TagReaderResult TagReaderClient::SaveSongRatingBlocking(const std::string &filename, float rating) {
  const TagReaderResult path = PathResult(filename);
  if (!path.success()) {
    return path;
  }
  return tagreader_->SaveRating(filename, rating) ? TagReaderResult{TagReaderResult::ErrorCode::Success}
                                                  : TagReaderResult{TagReaderResult::ErrorCode::FileSaveError};
}

TagReaderReplyPtr TagReaderClient::SaveSongRatingAsync(const std::string &filename, float rating) {
  auto reply = std::make_shared<TagReaderReply>(filename);
  auto request = TagReaderSaveRatingRequest::Create(filename);
  request->rating = rating;
  request->reply = reply;
  EnqueueRequest(request);
  return reply;
}

void TagReaderClient::SaveSongsPlaycountAsync(const SongList &songs) {
  for (const Song &song : songs) {
    const std::string path = FileUtils::PathFromUri(song.url());
    if (!path.empty()) {
      SaveSongPlaycountAsync(path, song.playcount());
    }
  }
}

void TagReaderClient::SaveSongsRatingAsync(const SongList &songs) {
  for (const Song &song : songs) {
    const std::string path = FileUtils::PathFromUri(song.url());
    if (!path.empty()) {
      SaveSongRatingAsync(path, song.rating());
    }
  }
}

void TagReaderClient::ProcessRequest(const TagReaderRequestPtr &request) {
  if (!request) {
    return;
  }
  TagReaderResult result{TagReaderResult::ErrorCode::Unsupported};
  if (auto typed = std::dynamic_pointer_cast<TagReaderIsMediaFileRequest>(request)) {
    const TagReaderResult path = PathResult(typed->filename);
    result = !path.success() ? path
                             : (IsMediaFileBlocking(typed->filename) ? TagReaderResult{TagReaderResult::ErrorCode::Success}
                                                                     : TagReaderResult{TagReaderResult::ErrorCode::Unsupported});
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderReadFileRequest>(request)) {
    Song song;
    result = ReadFileBlocking(typed->filename, &song);
    if (auto reply = std::dynamic_pointer_cast<TagReaderReadFileReply>(request->reply)) {
      reply->set_song(song);
    }
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderReadStreamRequest>(request)) {
    Song song;
    result = ReadStreamBlocking(typed->url, typed->filename, typed->size, typed->mtime, typed->token_type, typed->access_token, &song);
    if (auto reply = std::dynamic_pointer_cast<TagReaderReadStreamReply>(request->reply)) {
      reply->set_song(song);
    }
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderWriteFileRequest>(request)) {
    result = WriteFileBlocking(typed->filename, typed->song, typed->save_tags_options, typed->save_tag_cover_data, typed->tag_id3v2_version);
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderLoadCoverDataRequest>(request)) {
    std::vector<unsigned char> data;
    result = LoadCoverDataBlocking(typed->filename, &data);
    if (auto reply = std::dynamic_pointer_cast<TagReaderLoadCoverDataReply>(request->reply)) {
      reply->set_data(data);
    }
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderLoadCoverImageRequest>(request)) {
    std::vector<unsigned char> data;
    result = LoadCoverDataBlocking(typed->filename, &data);
    if (auto reply = std::dynamic_pointer_cast<TagReaderLoadCoverDataReply>(request->reply)) {
      reply->set_data(data);
    }
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderSaveCoverRequest>(request)) {
    result = SaveCoverBlocking(typed->filename, typed->save_tag_cover_data);
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderSavePlaycountRequest>(request)) {
    result = SaveSongPlaycountBlocking(typed->filename, typed->playcount);
  } else if (auto typed = std::dynamic_pointer_cast<TagReaderSaveRatingRequest>(request)) {
    result = SaveSongRatingBlocking(typed->filename, typed->rating);
  }
  if (request->reply) {
    request->reply->set_result(result);
    request->reply->Finish();
  }
}
