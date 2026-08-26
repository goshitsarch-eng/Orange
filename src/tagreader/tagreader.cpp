#include "tagreader/tagreader.h"

#include "core/logging.h"
#include "utilities/fileutils.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#ifdef HAVE_TAGLIB_DSFFILE
#  include <taglib/dsffile.h>
#endif

#include <sys/stat.h>

#include <algorithm>

namespace {

std::string FromTagLib(const TagLib::String &value) { return value.to8Bit(true); }

TagLib::String ToTagLib(const std::string &value) { return TagLib::String(value, TagLib::String::UTF8); }

}  // namespace

void TagReader::ApplyFileInfo(Song *song, const std::string &filename) const {
  song->set_url(FileUtils::UriFromPath(filename));
  song->set_basefilename(FileUtils::BaseName(filename));
  song->set_filetype(Song::FiletypeByFilename(filename));
  struct stat st {};
  if (stat(filename.c_str(), &st) == 0) {
    song->set_filesize(st.st_size);
    song->set_mtime(st.st_mtime);
    song->set_ctime(st.st_ctime);
  }
}

bool TagReader::IsMediaFile(const std::string &filename) const {
  if (!Song::IsAudioFile(filename)) {
    return false;
  }
  TagLib::FileRef file(filename.c_str(), false);
  return !file.isNull() && file.file() && file.file()->isValid();
}

Song TagReader::ReadFile(const std::string &filename) const {
  Song song(Song::Source::LocalFile);
  TagLib::FileRef file(filename.c_str(), true, TagLib::AudioProperties::Accurate);
  if (file.isNull() || !file.tag()) {
    ApplyFileInfo(&song, filename);
    return song;
  }

  const TagLib::Tag *tag = file.tag();
  song.set_valid(true);
  song.set_title(FromTagLib(tag->title()));
  song.set_artist(FromTagLib(tag->artist()));
  song.set_album(FromTagLib(tag->album()));
  song.set_comment(FromTagLib(tag->comment()));
  song.set_genre(FromTagLib(tag->genre()));
  song.set_year(static_cast<int>(tag->year()));
  song.set_track(static_cast<int>(tag->track()));

  const TagLib::PropertyMap properties = file.file()->properties();
  auto take = [&properties](const char *key) -> std::string {
    const auto it = properties.find(key);
    if (it == properties.end() || it->second.isEmpty()) {
      return {};
    }
    return FromTagLib(it->second.front());
  };

  song.set_albumartist(take("ALBUMARTIST"));
  song.set_composer(take("COMPOSER"));
  song.set_performer(take("PERFORMER"));
  song.set_grouping(take("GROUPING"));
  song.set_lyrics(take("LYRICS"));
  if (properties.contains("DISCNUMBER") && !properties["DISCNUMBER"].isEmpty()) {
    song.set_disc(properties["DISCNUMBER"].front().toInt());
  }
  if (properties.contains("ORIGINALYEAR") && !properties["ORIGINALYEAR"].isEmpty()) {
    song.set_originalyear(properties["ORIGINALYEAR"].front().toInt());
  }
  if (properties.contains("BPM") && !properties["BPM"].isEmpty()) {
    try {
      song.set_bpm(std::stof(FromTagLib(properties["BPM"].front())));
    } catch (...) {
    }
  }
  song.set_mood(take("MOOD"));
  song.set_initial_key(take("INITIALKEY"));
  song.set_musicbrainz_album_id(take("MUSICBRAINZ_ALBUMID"));
  song.set_musicbrainz_artist_id(take("MUSICBRAINZ_ARTISTID"));
  song.set_musicbrainz_album_artist_id(take("MUSICBRAINZ_ALBUMARTISTID"));
  song.set_musicbrainz_recording_id(take("MUSICBRAINZ_TRACKID"));
  song.set_acoustid_id(take("ACOUSTID_ID"));
  song.set_acoustid_fingerprint(take("ACOUSTID_FINGERPRINT"));

  if (file.audioProperties()) {
    const TagLib::AudioProperties *properties_audio = file.audioProperties();
    song.set_length_nanosec(static_cast<int64_t>(properties_audio->lengthInMilliseconds()) * 1000000LL);
    song.set_bitrate(properties_audio->bitrate());
    song.set_samplerate(properties_audio->sampleRate());
  }

  ApplyFileInfo(&song, filename);
  if (song.title().empty()) {
    song.set_title(FileUtils::BaseName(filename));
  }
  return song;
}

bool TagReader::WriteFile(const Song &song) const {
  const std::string filename = FileUtils::PathFromUri(song.url());
  TagLib::FileRef file(filename.c_str());
  if (file.isNull() || !file.tag()) {
    return false;
  }
  file.tag()->setTitle(ToTagLib(song.title()));
  file.tag()->setArtist(ToTagLib(song.artist()));
  file.tag()->setAlbum(ToTagLib(song.album()));
  file.tag()->setComment(ToTagLib(song.comment()));
  file.tag()->setGenre(ToTagLib(song.genre()));
  file.tag()->setYear(song.year() > 0 ? static_cast<unsigned>(song.year()) : 0);
  file.tag()->setTrack(song.track() > 0 ? static_cast<unsigned>(song.track()) : 0);

  TagLib::PropertyMap properties = file.file()->properties();
  auto set_or_remove = [&properties](const char *key, const std::string &value) {
    if (value.empty()) {
      properties.erase(key);
    } else {
      properties.replace(key, TagLib::StringList(ToTagLib(value)));
    }
  };
  set_or_remove("ALBUMARTIST", song.albumartist());
  set_or_remove("COMPOSER", song.composer());
  set_or_remove("PERFORMER", song.performer());
  set_or_remove("GROUPING", song.grouping());
  set_or_remove("LYRICS", song.lyrics());
  file.file()->setProperties(properties);
  return file.save();
}

TagReader::CoverData TagReader::LoadCoverData(const std::string &filename) const {
  CoverData cover;
  {
    TagLib::MPEG::File mpeg(filename.c_str());
    if (mpeg.isValid() && mpeg.ID3v2Tag()) {
      const auto frames = mpeg.ID3v2Tag()->frameListMap()["APIC"];
      if (!frames.isEmpty()) {
        auto *picture = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());
        if (picture) {
          const TagLib::ByteVector data = picture->picture();
          cover.data.assign(data.begin(), data.end());
          cover.mime_type = FromTagLib(picture->mimeType());
          return cover;
        }
      }
    }
  }
  {
    TagLib::FLAC::File flac(filename.c_str());
    if (flac.isValid()) {
      const auto pictures = flac.pictureList();
      if (!pictures.isEmpty() && pictures.front()) {
        const TagLib::ByteVector data = pictures.front()->data();
        cover.data.assign(data.begin(), data.end());
        cover.mime_type = FromTagLib(pictures.front()->mimeType());
      }
    }
  }
  return cover;
}

bool TagReader::SaveCover(const std::string &filename, const CoverData &cover) const {
  if (cover.data.empty()) {
    return false;
  }
  TagLib::ByteVector bytes(reinterpret_cast<const char *>(cover.data.data()), static_cast<unsigned>(cover.data.size()));
  {
    TagLib::MPEG::File mpeg(filename.c_str());
    if (mpeg.isValid()) {
      TagLib::ID3v2::Tag *tag = mpeg.ID3v2Tag(true);
      tag->removeFrames("APIC");
      auto *frame = new TagLib::ID3v2::AttachedPictureFrame();
      frame->setMimeType(ToTagLib(cover.mime_type.empty() ? "image/jpeg" : cover.mime_type));
      frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
      frame->setPicture(bytes);
      tag->addFrame(frame);
      return mpeg.save();
    }
  }
  {
    TagLib::FLAC::File flac(filename.c_str());
    if (flac.isValid()) {
      flac.removePictures();
      auto *picture = new TagLib::FLAC::Picture();
      picture->setMimeType(ToTagLib(cover.mime_type.empty() ? "image/jpeg" : cover.mime_type));
      picture->setType(TagLib::FLAC::Picture::FrontCover);
      picture->setData(bytes);
      flac.addPicture(picture);
      return flac.save();
    }
  }
  return false;
}

bool TagReader::SavePlaycount(const std::string &filename, unsigned playcount) const {
  TagLib::FileRef file(filename.c_str());
  if (file.isNull() || !file.file()) {
    return false;
  }
  TagLib::PropertyMap properties = file.file()->properties();
  properties.replace("PLAYCOUNT", TagLib::StringList(TagLib::String::number(static_cast<int>(playcount))));
  file.file()->setProperties(properties);
  return file.save();
}

bool TagReader::SaveRating(const std::string &filename, float rating) const {
  TagLib::FileRef file(filename.c_str());
  if (file.isNull() || !file.file()) {
    return false;
  }
  TagLib::PropertyMap properties = file.file()->properties();
  properties.replace("RATING", TagLib::StringList(TagLib::String::number(rating)));
  file.file()->setProperties(properties);
  return file.save();
}
