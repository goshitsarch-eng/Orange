#include "tagreader/tagreader.h"

#include "core/logging.h"
#include "tagreader/albumcovertagdata.h"
#include "tagreader/tagreadergme.h"
#include "tagreader/streamtagreader.h"
#include "utilities/fileutils.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/popularimeterframe.h>
#include <taglib/textidentificationframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/xiphcomment.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/speexfile.h>
#include <taglib/wavfile.h>
#include <taglib/aifffile.h>
#include <taglib/apefile.h>
#include <taglib/apetag.h>
#include <taglib/wavpackfile.h>
#include <taglib/mpcfile.h>
#include <taglib/asffile.h>
#include <taglib/asftag.h>
#ifdef HAVE_TAGLIB_DSFFILE
#  include <taglib/dsffile.h>
#endif

#include <sys/stat.h>

#include <algorithm>
#include <string>

namespace {

constexpr char kFmpsRating[] = "FMPS_RATING";
constexpr char kFmpsPlaycount[] = "FMPS_PLAYCOUNT";
constexpr char kId3FmpsRating[] = "FMPS_Rating";
constexpr char kId3FmpsPlaycount[] = "FMPS_Playcount";
constexpr char kMp4Cover[] = "covr";
constexpr char kMp4FmpsRating[] = "----:com.apple.iTunes:FMPS_Rating";
constexpr char kMp4FmpsPlaycount[] = "----:com.apple.iTunes:FMPS_Playcount";
constexpr char kApeCover[] = "COVER ART (FRONT)";

std::string FromTagLib(const TagLib::String &value) { return value.to8Bit(true); }

TagLib::String ToTagLib(const std::string &value) { return TagLib::String(value, TagLib::String::UTF8); }

void ReadFromFileRef(TagLib::FileRef *file, Song *song);

float ParseRatingString(const std::string &value) {
  if (value.empty()) {
    return -1.0f;
  }
  try {
    const float rating = std::stof(value);
    return rating > 0.0f && rating <= 1.0f ? rating : -1.0f;
  } catch (...) {
    return -1.0f;
  }
}

void ApplyRating(Song *song, float rating) {
  if (song && song->rating() <= 0 && rating > 0) {
    song->set_rating(rating);
  }
}

void ApplyPlaycount(Song *song, int playcount) {
  if (song && song->playcount() == 0 && playcount > 0) {
    song->set_playcount(static_cast<unsigned>(playcount));
  }
}

TagReader::CoverData CoverFromBytes(const TagLib::ByteVector &bytes, const TagLib::String &mime) {
  TagReader::CoverData cover;
  if (bytes.size() <= 0) {
    return cover;
  }
  cover.data.assign(bytes.begin(), bytes.end());
  cover.mime_type = FromTagLib(mime);
  if (cover.mime_type.empty()) {
    cover.mime_type = AlbumCoverTagData::GuessMimeType(cover.data);
  }
  return cover;
}

TagReader::CoverData CoverFromId3v2(TagLib::ID3v2::Tag *tag) {
  TagReader::CoverData cover;
  if (!tag) {
    return cover;
  }
  const auto frames = tag->frameListMap()["APIC"];
  if (frames.isEmpty()) {
    return cover;
  }
  if (auto *picture = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front())) {
    return CoverFromBytes(picture->picture(), picture->mimeType());
  }
  return cover;
}

TagReader::CoverData CoverFromFlacPictures(const TagLib::List<TagLib::FLAC::Picture *> &pictures) {
  TagReader::CoverData cover;
  for (TagLib::FLAC::Picture *picture : pictures) {
    if (!picture || picture->data().size() <= 0) {
      continue;
    }
    if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
      return CoverFromBytes(picture->data(), picture->mimeType());
    }
    if (cover.data.empty()) {
      cover = CoverFromBytes(picture->data(), picture->mimeType());
    }
  }
  return cover;
}

TagReader::CoverData CoverFromApe(TagLib::APE::Tag *tag) {
  TagReader::CoverData cover;
  if (!tag) {
    return cover;
  }
  const auto it = tag->itemListMap().find(kApeCover);
  if (it == tag->itemListMap().end()) {
    return cover;
  }
  const TagLib::ByteVector data = it->second.binaryData();
  const int pos = data.find('\0') + 1;
  if (pos > 0 && static_cast<unsigned>(pos) < data.size()) {
    cover.data.assign(data.begin() + pos, data.end());
    cover.mime_type = AlbumCoverTagData::GuessMimeType(cover.data);
  }
  return cover;
}

TagReader::CoverData CoverFromMp4(TagLib::MP4::Tag *tag) {
  TagReader::CoverData cover;
  if (!tag || !tag->item(kMp4Cover).isValid()) {
    return cover;
  }
  const TagLib::MP4::CoverArtList art = tag->item(kMp4Cover).toCoverArtList();
  if (art.isEmpty()) {
    return cover;
  }
  cover.data.assign(art.front().data().begin(), art.front().data().end());
  switch (art.front().format()) {
    case TagLib::MP4::CoverArt::PNG:
      cover.mime_type = "image/png";
      break;
    case TagLib::MP4::CoverArt::BMP:
      cover.mime_type = "image/bmp";
      break;
    case TagLib::MP4::CoverArt::GIF:
      cover.mime_type = "image/gif";
      break;
    default:
      cover.mime_type = "image/jpeg";
      break;
  }
  return cover;
}

TagReader::CoverData CoverFromFile(TagLib::File *file) {
  if (!file) {
    return {};
  }
  if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
    TagReader::CoverData cover = CoverFromFlacPictures(flac->pictureList());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
    TagReader::CoverData cover = CoverFromId3v2(mpeg->ID3v2Tag());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
    TagReader::CoverData cover = CoverFromMp4(mp4->tag());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *xiph = dynamic_cast<TagLib::Ogg::XiphComment *>(file->tag())) {
    TagReader::CoverData cover = CoverFromFlacPictures(xiph->pictureList());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file)) {
    if (wav->hasID3v2Tag()) {
      TagReader::CoverData cover = CoverFromId3v2(wav->ID3v2Tag());
      if (!cover.data.empty()) {
        return cover;
      }
    }
  }
  if (auto *aiff = dynamic_cast<TagLib::RIFF::AIFF::File *>(file)) {
    if (aiff->hasID3v2Tag()) {
      TagReader::CoverData cover = CoverFromId3v2(aiff->tag());
      if (!cover.data.empty()) {
        return cover;
      }
    }
  }
  if (auto *ape = dynamic_cast<TagLib::APE::File *>(file)) {
    TagReader::CoverData cover = CoverFromApe(ape->APETag());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *wavpack = dynamic_cast<TagLib::WavPack::File *>(file)) {
    TagReader::CoverData cover = CoverFromApe(wavpack->APETag());
    if (!cover.data.empty()) {
      return cover;
    }
  }
  if (auto *mpc = dynamic_cast<TagLib::MPC::File *>(file)) {
    return CoverFromApe(mpc->APETag());
  }
  return {};
}

void ReadId3v2Extras(TagLib::ID3v2::Tag *tag, Song *song) {
  if (!tag || !song) {
    return;
  }
  if (auto *fmps_playcount = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, kId3FmpsPlaycount)) {
    const TagLib::StringList fields = fmps_playcount->fieldList();
    if (fields.size() > 1) {
      ApplyPlaycount(song, fields[1].toInt());
    }
  }
  if (auto *fmps_rating = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, kId3FmpsRating)) {
    const TagLib::StringList fields = fmps_rating->fieldList();
    if (fields.size() > 1) {
      ApplyRating(song, ParseRatingString(FromTagLib(fields[1])));
    }
  }
  const auto popm = tag->frameListMap()["POPM"];
  for (auto *frame : popm) {
    if (auto *popular = dynamic_cast<TagLib::ID3v2::PopularimeterFrame *>(frame)) {
      ApplyPlaycount(song, static_cast<int>(popular->counter()));
      if (popular->rating() > 0) {
        ApplyRating(song, TagReaderBase::ConvertPOPMRating(popular->rating()));
      }
    }
  }
}

void SetId3v2Rating(TagLib::ID3v2::Tag *tag, float rating) {
  if (!tag) {
    return;
  }
  tag->removeFrames("POPM");
  if (auto *existing = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, kId3FmpsRating)) {
    tag->removeFrame(existing);
  }
  if (rating <= 0) {
    return;
  }
  auto *popm = new TagLib::ID3v2::PopularimeterFrame();
  popm->setEmail("strawberry");
  popm->setRating(TagReaderBase::ConvertToPOPMRating(rating));
  tag->addFrame(popm);
  auto *fmps = new TagLib::ID3v2::UserTextIdentificationFrame(TagLib::String::UTF8);
  fmps->setDescription(kId3FmpsRating);
  fmps->setText(TagLib::String::number(rating));
  tag->addFrame(fmps);
}

void SetId3v2Cover(TagLib::ID3v2::Tag *tag, const TagLib::ByteVector &bytes, const std::string &mime) {
  if (!tag) {
    return;
  }
  tag->removeFrames("APIC");
  if (bytes.size() <= 0) {
    return;
  }
  auto *frame = new TagLib::ID3v2::AttachedPictureFrame();
  frame->setMimeType(ToTagLib(mime.empty() ? "image/jpeg" : mime));
  frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
  frame->setPicture(bytes);
  tag->addFrame(frame);
}

void SetXiphCover(TagLib::Ogg::XiphComment *comment, const TagLib::ByteVector &bytes, const std::string &mime) {
  if (!comment) {
    return;
  }
  comment->removeAllPictures();
  if (bytes.size() <= 0) {
    return;
  }
  auto *picture = new TagLib::FLAC::Picture();
  picture->setType(TagLib::FLAC::Picture::FrontCover);
  picture->setMimeType(ToTagLib(mime.empty() ? "image/jpeg" : mime));
  picture->setData(bytes);
  comment->addPicture(picture);
}

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
  if (TagReaderGME::IsSupported(filename)) {
    return true;
  }
  TagLib::FileRef file(filename.c_str(), false);
  return !file.isNull() && file.file() && file.file()->isValid();
}

Song TagReader::ReadFile(const std::string &filename) const {
  Song song(Song::Source::LocalFile);
  if (TagReaderGME::IsSupported(filename) && TagReaderGME::ReadFile(filename, &song)) {
    ApplyFileInfo(&song, filename);
    if (song.title().empty()) {
      song.set_title(FileUtils::BaseName(filename));
    }
    return song;
  }
  TagLib::FileRef file(filename.c_str(), true, TagLib::AudioProperties::Accurate);
  if (file.isNull() || !file.tag()) {
    ApplyFileInfo(&song, filename);
    return song;
  }
  ReadFromFileRef(&file, &song);
  ApplyFileInfo(&song, filename);
  if (song.title().empty()) {
    song.set_title(FileUtils::BaseName(filename));
  }
  return song;
}

namespace {

void ReadFromFileRef(TagLib::FileRef *file, Song *song) {
  if (!file || file->isNull() || !file->tag()) {
    return;
  }
  const TagLib::Tag *tag = file->tag();
  song->set_valid(true);
  song->set_title(FromTagLib(tag->title()));
  song->set_artist(FromTagLib(tag->artist()));
  song->set_album(FromTagLib(tag->album()));
  song->set_comment(FromTagLib(tag->comment()));
  song->set_genre(FromTagLib(tag->genre()));
  song->set_year(static_cast<int>(tag->year()));
  song->set_track(static_cast<int>(tag->track()));

  const TagLib::PropertyMap properties = file->file()->properties();
  auto take = [&properties](const char *key) -> std::string {
    const auto it = properties.find(key);
    if (it == properties.end() || it->second.isEmpty()) {
      return {};
    }
    return FromTagLib(it->second.front());
  };

  song->set_albumartist(take("ALBUMARTIST"));
  song->set_composer(take("COMPOSER"));
  song->set_performer(take("PERFORMER"));
  song->set_grouping(take("GROUPING"));
  song->set_lyrics(take("LYRICS"));
  song->set_titlesort(take("TITLESORT"));
  song->set_albumsort(take("ALBUMSORT"));
  song->set_artistsort(take("ARTISTSORT"));
  song->set_albumartistsort(take("ALBUMARTISTSORT"));
  song->set_composersort(take("COMPOSERSORT"));
  song->set_performersort(take("PERFORMERSORT"));
  {
    const std::string compilation = take("COMPILATION");
    song->set_compilation(compilation == "1" || compilation == "true" || compilation == "True");
  }
  if (properties.contains("DISCNUMBER") && !properties["DISCNUMBER"].isEmpty()) {
    song->set_disc(properties["DISCNUMBER"].front().toInt());
  }
  if (properties.contains("ORIGINALYEAR") && !properties["ORIGINALYEAR"].isEmpty()) {
    song->set_originalyear(properties["ORIGINALYEAR"].front().toInt());
  }
  if (properties.contains("BPM") && !properties["BPM"].isEmpty()) {
    try {
      song->set_bpm(std::stof(FromTagLib(properties["BPM"].front())));
    } catch (...) {
    }
  }
  song->set_mood(take("MOOD"));
  song->set_initial_key(take("INITIALKEY"));
  song->set_musicbrainz_album_id(take("MUSICBRAINZ_ALBUMID"));
  song->set_musicbrainz_artist_id(take("MUSICBRAINZ_ARTISTID"));
  song->set_musicbrainz_album_artist_id(take("MUSICBRAINZ_ALBUMARTISTID"));
  song->set_musicbrainz_recording_id(take("MUSICBRAINZ_TRACKID"));
  song->set_acoustid_id(take("ACOUSTID_ID"));
  song->set_acoustid_fingerprint(take("ACOUSTID_FINGERPRINT"));
  ApplyRating(song, ParseRatingString(take("FMPS_RATING")));
  if (song->rating() <= 0) {
    ApplyRating(song, ParseRatingString(take("RATING")));
  }
  if (properties.contains("FMPS_PLAYCOUNT") && !properties["FMPS_PLAYCOUNT"].isEmpty()) {
    ApplyPlaycount(song, properties["FMPS_PLAYCOUNT"].front().toInt());
  } else if (properties.contains("PLAYCOUNT") && !properties["PLAYCOUNT"].isEmpty()) {
    ApplyPlaycount(song, properties["PLAYCOUNT"].front().toInt());
  }

  if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file->file())) {
    ReadId3v2Extras(mpeg->ID3v2Tag(), song);
  } else if (auto *wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file->file())) {
    if (wav->hasID3v2Tag()) {
      ReadId3v2Extras(wav->ID3v2Tag(), song);
    }
  } else if (auto *aiff = dynamic_cast<TagLib::RIFF::AIFF::File *>(file->file())) {
    if (aiff->hasID3v2Tag()) {
      ReadId3v2Extras(aiff->tag(), song);
    }
  } else if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file->file())) {
    if (TagLib::MP4::Tag *tag = mp4->tag()) {
      if (tag->contains(kMp4FmpsRating)) {
        ApplyRating(song, ParseRatingString(FromTagLib(tag->item(kMp4FmpsRating).toStringList().toString())));
      }
      if (tag->contains(kMp4FmpsPlaycount)) {
        ApplyPlaycount(song, tag->item(kMp4FmpsPlaycount).toStringList().toString().toInt());
      }
    }
  }

  if (!CoverFromFile(file->file()).data.empty()) {
    song->set_art_embedded(true);
  }

  if (file->audioProperties()) {
    const TagLib::AudioProperties *properties_audio = file->audioProperties();
    song->set_length_nanosec(static_cast<int64_t>(properties_audio->lengthInMilliseconds()) * 1000000LL);
    song->set_bitrate(properties_audio->bitrate());
    song->set_samplerate(properties_audio->sampleRate());
  }
}

}  // namespace

Song TagReader::ReadStream(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime, const std::string &token_type,
                           const std::string &access_token) const {
  StreamTagReader stream(url, filename, static_cast<TagLibLengthType>(size), token_type, access_token);
  return ReadStream(&stream, url, filename, size, mtime);
}

Song TagReader::ReadStream(StreamTagReader *stream, const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime) const {
  Song song(Song::Source::Stream);
  song.set_url(url);
  song.set_basefilename(FileUtils::BaseName(filename));
  song.set_filesize(static_cast<int64_t>(size));
  song.set_ctime(static_cast<int64_t>(mtime));
  song.set_mtime(static_cast<int64_t>(mtime));
  song.set_filetype(Song::FiletypeByFilename(filename));
  if (!stream) {
    return song;
  }
  stream->PreCache();
  if (stream->num_requests() > 2) {
    LogWarning("Total requests for stream %s: %d cached %zu", filename.c_str(), stream->num_requests(),
               static_cast<size_t>(stream->cached_bytes()));
  }
  TagLib::FileRef file(stream, true, TagLib::AudioProperties::Accurate);
  if (file.isNull() || !file.tag()) {
    LogError("TagLib could not open stream %s", filename.c_str());
    return song;
  }
  ReadFromFileRef(&file, &song);
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
  set_or_remove("TITLESORT", song.titlesort());
  set_or_remove("ALBUMSORT", song.albumsort());
  set_or_remove("ARTISTSORT", song.artistsort());
  set_or_remove("ALBUMARTISTSORT", song.albumartistsort());
  set_or_remove("COMPOSERSORT", song.composersort());
  set_or_remove("PERFORMERSORT", song.performersort());
  set_or_remove("MOOD", song.mood());
  set_or_remove("INITIALKEY", song.initial_key());
  set_or_remove("COMPILATION", song.compilation() ? "1" : "");
  set_or_remove("DISCNUMBER", song.disc() > 0 ? std::to_string(song.disc()) : "");
  set_or_remove("ORIGINALYEAR", song.originalyear() > 0 ? std::to_string(song.originalyear()) : "");
  set_or_remove("BPM", song.bpm() > 0 ? std::to_string(song.bpm()) : "");
  file.file()->setProperties(properties);
  return file.save();
}

TagReader::CoverData TagReader::LoadCoverData(const std::string &filename) const {
  TagLib::FileRef file(filename.c_str(), false);
  if (file.isNull() || !file.file()) {
    return {};
  }
  return CoverFromFile(file.file());
}

bool TagReader::ClearCover(const std::string &filename) const { return SaveCover(filename, {}); }

bool TagReader::SaveCover(const std::string &filename, const CoverData &cover) const {
  TagLib::ByteVector bytes(reinterpret_cast<const char *>(cover.data.data()), static_cast<unsigned>(cover.data.size()));
  const std::string mime = cover.mime_type.empty() ? AlbumCoverTagData::GuessMimeType(cover.data) : cover.mime_type;
  {
    TagLib::MPEG::File mpeg(filename.c_str());
    if (mpeg.isValid()) {
      SetId3v2Cover(mpeg.ID3v2Tag(true), bytes, mime);
      return mpeg.save();
    }
  }
  {
    TagLib::FLAC::File flac(filename.c_str());
    if (flac.isValid()) {
      flac.removePictures();
      if (!cover.data.empty()) {
        auto *picture = new TagLib::FLAC::Picture();
        picture->setMimeType(ToTagLib(mime.empty() ? "image/jpeg" : mime));
        picture->setType(TagLib::FLAC::Picture::FrontCover);
        picture->setData(bytes);
        flac.addPicture(picture);
      }
      return flac.save();
    }
  }
  {
    TagLib::MP4::File mp4(filename.c_str());
    if (mp4.isValid() && mp4.tag()) {
      if (cover.data.empty()) {
        mp4.tag()->removeItem(kMp4Cover);
      } else {
        TagLib::MP4::CoverArt::Format format = TagLib::MP4::CoverArt::JPEG;
        if (mime == "image/png") {
          format = TagLib::MP4::CoverArt::PNG;
        } else if (mime == "image/gif") {
          format = TagLib::MP4::CoverArt::GIF;
        }
        TagLib::MP4::CoverArtList list;
        list.append(TagLib::MP4::CoverArt(format, bytes));
        mp4.tag()->setItem(kMp4Cover, TagLib::MP4::Item(list));
      }
      return mp4.save();
    }
  }
  {
    TagLib::RIFF::WAV::File wav(filename.c_str());
    if (wav.isValid()) {
      SetId3v2Cover(wav.ID3v2Tag(), bytes, mime);
      return wav.save();
    }
  }
  {
    TagLib::RIFF::AIFF::File aiff(filename.c_str());
    if (aiff.isValid()) {
      SetId3v2Cover(aiff.tag(), bytes, mime);
      return aiff.save();
    }
  }
  {
    TagLib::Ogg::Vorbis::File vorbis(filename.c_str());
    if (vorbis.isValid() && vorbis.tag()) {
      SetXiphCover(vorbis.tag(), bytes, mime);
      return vorbis.save();
    }
  }
  {
    TagLib::Ogg::Opus::File opus(filename.c_str());
    if (opus.isValid() && opus.tag()) {
      SetXiphCover(opus.tag(), bytes, mime);
      return opus.save();
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
  properties.replace(kFmpsPlaycount, TagLib::StringList(TagLib::String::number(static_cast<int>(playcount))));
  file.file()->setProperties(properties);
  if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file.file())) {
    if (TagLib::ID3v2::Tag *tag = mpeg->ID3v2Tag(true)) {
      if (auto *existing = TagLib::ID3v2::UserTextIdentificationFrame::find(tag, kId3FmpsPlaycount)) {
        tag->removeFrame(existing);
      }
      auto *fmps = new TagLib::ID3v2::UserTextIdentificationFrame(TagLib::String::UTF8);
      fmps->setDescription(kId3FmpsPlaycount);
      fmps->setText(TagLib::String::number(static_cast<int>(playcount)));
      tag->addFrame(fmps);
    }
  }
  return file.save();
}

bool TagReader::SaveRating(const std::string &filename, float rating) const {
  TagLib::FileRef file(filename.c_str());
  if (file.isNull() || !file.file()) {
    return false;
  }
  if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file.file())) {
    SetId3v2Rating(mpeg->ID3v2Tag(true), rating);
  } else if (auto *wav = dynamic_cast<TagLib::RIFF::WAV::File *>(file.file())) {
    SetId3v2Rating(wav->ID3v2Tag(), rating);
  } else if (auto *aiff = dynamic_cast<TagLib::RIFF::AIFF::File *>(file.file())) {
    SetId3v2Rating(aiff->tag(), rating);
  } else if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file.file())) {
    if (TagLib::MP4::Tag *tag = mp4->tag()) {
      if (rating > 0) {
        tag->setItem(kMp4FmpsRating, TagLib::MP4::Item(TagLib::StringList(TagLib::String::number(rating))));
      } else {
        tag->removeItem(kMp4FmpsRating);
      }
    }
  } else if (auto *xiph = dynamic_cast<TagLib::Ogg::XiphComment *>(file.file()->tag())) {
    if (rating > 0) {
      xiph->addField(kFmpsRating, TagLib::String::number(rating), true);
    } else {
      xiph->removeFields(kFmpsRating);
    }
  }
  TagLib::PropertyMap properties = file.file()->properties();
  if (rating > 0) {
    properties.replace("RATING", TagLib::StringList(TagLib::String::number(rating)));
    properties.replace(kFmpsRating, TagLib::StringList(TagLib::String::number(rating)));
  } else {
    properties.erase("RATING");
    properties.erase(kFmpsRating);
  }
  file.file()->setProperties(properties);
  return file.save();
}
