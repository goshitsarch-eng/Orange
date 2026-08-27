#ifndef STRAWBERRY_TAGREADERCLIENT_H
#define STRAWBERRY_TAGREADERCLIENT_H

#include "core/song.h"
#include "tagreader/savetagcoverdata.h"
#include "tagreader/savetagsoptions.h"
#include "tagreader/tagid3v2version.h"
#include "tagreader/tagreader.h"
#include "tagreader/tagreaderismediafilerequest.h"
#include "tagreader/tagreaderloadcoverdatareply.h"
#include "tagreader/tagreaderloadcoverdatarequest.h"
#include "tagreader/tagreaderloadcoverimagereply.h"
#include "tagreader/tagreaderloadcoverimagerequest.h"
#include "tagreader/tagreaderreadfilereply.h"
#include "tagreader/tagreaderreadfilerequest.h"
#include "tagreader/tagreaderreadstreamreply.h"
#include "tagreader/tagreaderreadstreamrequest.h"
#include "tagreader/tagreaderreply.h"
#include "tagreader/tagreaderrequest.h"
#include "tagreader/tagreaderresult.h"
#include "tagreader/tagreadersavecoverrequest.h"
#include "tagreader/tagreadersaveplaycountrequest.h"
#include "tagreader/tagreadersaveratingrequest.h"
#include "tagreader/tagreaderwritefilerequest.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

class TagReaderClient {
 public:
  explicit TagReaderClient(TagReader *tagreader = nullptr);

  using SaveOption = SaveTagsOption;
  using SaveOptions = SaveTagsOptions;

  bool HaveRequests() const;
  void EnqueueRequest(TagReaderRequestPtr request);
  void ProcessRequests();
  bool ProcessNext();
  void ProcessRequest(const TagReaderRequestPtr &request);
  void Clear();

  TagReaderResult PathResult(const std::string &filename) const;

  bool IsMediaFileBlocking(const std::string &filename) const;
  TagReaderReplyPtr IsMediaFileAsync(const std::string &filename);

  TagReaderResult ReadFileBlocking(const std::string &filename, Song *song);
  TagReaderReadFileReplyPtr ReadFileAsync(const std::string &filename);

  TagReaderResult ReadStreamBlocking(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime,
                                     const std::string &token_type, const std::string &access_token, Song *song);
  TagReaderReadStreamReplyPtr ReadStreamAsync(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime,
                                              const std::string &token_type, const std::string &access_token);

  TagReaderResult WriteFileBlocking(const std::string &filename, const Song &song,
                                    SaveTagsOptions save_tags_options = static_cast<int>(SaveTagsOption::Tags),
                                    const SaveTagCoverData &save_tag_cover_data = {},
                                    TagID3v2Version tag_id3v2_version = TagID3v2Version::Default);
  TagReaderReplyPtr WriteFileAsync(const std::string &filename, const Song &song,
                                   SaveTagsOptions save_tags_options = static_cast<int>(SaveTagsOption::Tags),
                                   const SaveTagCoverData &save_tag_cover_data = {},
                                   TagID3v2Version tag_id3v2_version = TagID3v2Version::Default);

  TagReaderResult LoadCoverDataBlocking(const std::string &filename, std::vector<unsigned char> *data);
  TagReaderLoadCoverDataReplyPtr LoadCoverDataAsync(const std::string &filename);
  TagReaderLoadCoverImageReplyPtr LoadCoverImageAsync(const std::string &filename);

  TagReaderResult SaveCoverBlocking(const std::string &filename, const SaveTagCoverData &save_tag_cover_data);
  TagReaderReplyPtr SaveCoverAsync(const std::string &filename, const SaveTagCoverData &save_tag_cover_data);

  TagReaderResult SaveSongPlaycountBlocking(const std::string &filename, unsigned playcount);
  TagReaderReplyPtr SaveSongPlaycountAsync(const std::string &filename, unsigned playcount);

  TagReaderResult SaveSongRatingBlocking(const std::string &filename, float rating);
  TagReaderReplyPtr SaveSongRatingAsync(const std::string &filename, float rating);

  void SaveSongsPlaycountAsync(const SongList &songs);
  void SaveSongsRatingAsync(const SongList &songs);

 private:
  TagReaderRequestPtr DequeueRequest();

  TagReader owned_;
  TagReader *tagreader_;
  std::queue<TagReaderRequestPtr> requests_;
};

#endif
