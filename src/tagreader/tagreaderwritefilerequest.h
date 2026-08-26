#ifndef STRAWBERRY_TAGREADERWRITEFILEREQUEST_H
#define STRAWBERRY_TAGREADERWRITEFILEREQUEST_H

#include "core/song.h"
#include "tagreader/savetagcoverdata.h"
#include "tagreader/savetagsoptions.h"
#include "tagreader/tagid3v2version.h"
#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderWriteFileRequest : public TagReaderRequest {
 public:
  explicit TagReaderWriteFileRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderWriteFileRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderWriteFileRequest>(filename);
  }

  SaveTagsOptions save_tags_options = static_cast<int>(SaveTagsOption::Tags);
  Song song;
  SaveTagCoverData save_tag_cover_data;
  TagID3v2Version tag_id3v2_version = TagID3v2Version::Default;
};

using TagReaderWriteFileRequestPtr = std::shared_ptr<TagReaderWriteFileRequest>;

#endif
