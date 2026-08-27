#ifndef STRAWBERRY_SONG_H
#define STRAWBERRY_SONG_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Song {
 public:
  enum class Source {
    Unknown = 0,
    LocalFile = 1,
    Collection = 2,
    CDDA = 3,
    Device = 4,
    Stream = 5,
    Tidal = 6,
    Subsonic = 7,
    Qobuz = 8,
    SomaFM = 9,
    RadioParadise = 10,
    Spotify = 11,
    RadioBrowser = 12
  };

  enum class FileType {
    Unknown = 0,
    WAV = 1,
    FLAC = 2,
    WavPack = 3,
    OggFlac = 4,
    OggVorbis = 5,
    OggOpus = 6,
    OggSpeex = 7,
    MPEG = 8,
    MP4 = 9,
    ASF = 10,
    AIFF = 11,
    MPC = 12,
    TrueAudio = 13,
    DSF = 14,
    DSDIFF = 15,
    PCM = 16,
    APE = 17,
    MOD = 18,
    S3M = 19,
    XM = 20,
    IT = 21,
    SPC = 22,
    VGM = 23,
    ALAC = 24,
    CDDA = 90,
    Stream = 91
  };

  static const int kSourceCount = 16;
  static const std::vector<std::string> kAcceptedExtensions;
  static const std::vector<std::string> kRejectedExtensions;
  static const std::vector<std::string> kColumns;
  static const char *kColumnSpec;

  explicit Song(Source source = Source::Unknown);

  bool operator==(const Song &other) const;
  bool operator!=(const Song &other) const { return !(*this == other); }

  bool is_valid() const { return valid_; }
  void set_valid(bool v) { valid_ = v; }

  int id() const { return id_; }
  void set_id(int id) { id_ = id; }

  const std::string &title() const { return title_; }
  const std::string &titlesort() const { return titlesort_; }
  const std::string &album() const { return album_; }
  const std::string &albumsort() const { return albumsort_; }
  const std::string &artist() const { return artist_; }
  const std::string &artistsort() const { return artistsort_; }
  const std::string &albumartist() const { return albumartist_; }
  const std::string &albumartistsort() const { return albumartistsort_; }
  int track() const { return track_; }
  int disc() const { return disc_; }
  int year() const { return year_; }
  int originalyear() const { return originalyear_; }
  const std::string &genre() const { return genre_; }
  bool compilation() const { return compilation_; }
  const std::string &composer() const { return composer_; }
  const std::string &composersort() const { return composersort_; }
  const std::string &performer() const { return performer_; }
  const std::string &performersort() const { return performersort_; }
  const std::string &grouping() const { return grouping_; }
  const std::string &comment() const { return comment_; }
  const std::string &lyrics() const { return lyrics_; }

  const std::string &artist_id() const { return artist_id_; }
  const std::string &album_id() const { return album_id_; }
  const std::string &song_id() const { return song_id_; }

  int64_t beginning_nanosec() const { return beginning_nanosec_; }
  int64_t length_nanosec() const { return length_nanosec_; }
  int bitrate() const { return bitrate_; }
  int samplerate() const { return samplerate_; }
  int bitdepth() const { return bitdepth_; }

  Source source() const { return source_; }
  int directory_id() const { return directory_id_; }
  const std::string &url() const { return url_; }
  const std::string &stream_url() const { return stream_url_.empty() ? url_ : stream_url_; }
  const std::string &basefilename() const { return basefilename_; }
  FileType filetype() const { return filetype_; }
  int64_t filesize() const { return filesize_; }
  int64_t mtime() const { return mtime_; }
  int64_t ctime() const { return ctime_; }
  bool unavailable() const { return unavailable_; }
  bool skipped() const { return skipped_; }
  void set_skipped(bool v) { skipped_ = v; }

  const std::string &fingerprint() const { return fingerprint_; }
  unsigned playcount() const { return playcount_; }
  unsigned skipcount() const { return skipcount_; }
  int64_t lastplayed() const { return lastplayed_; }
  int64_t lastseen() const { return lastseen_; }

  bool art_embedded() const { return art_embedded_; }
  const std::string &art_automatic() const { return art_automatic_; }
  const std::string &art_manual() const { return art_manual_; }
  bool art_unset() const { return art_unset_; }
  const std::string &cue_path() const { return cue_path_; }

  float rating() const { return rating_; }
  float bpm() const { return bpm_; }
  const std::string &mood() const { return mood_; }
  const std::string &initial_key() const { return initial_key_; }

  const std::string &acoustid_id() const { return acoustid_id_; }
  const std::string &acoustid_fingerprint() const { return acoustid_fingerprint_; }
  const std::string &musicbrainz_album_artist_id() const { return musicbrainz_album_artist_id_; }
  const std::string &musicbrainz_artist_id() const { return musicbrainz_artist_id_; }
  const std::string &musicbrainz_original_artist_id() const { return musicbrainz_original_artist_id_; }
  const std::string &musicbrainz_album_id() const { return musicbrainz_album_id_; }
  const std::string &musicbrainz_original_album_id() const { return musicbrainz_original_album_id_; }
  const std::string &musicbrainz_recording_id() const { return musicbrainz_recording_id_; }
  const std::string &musicbrainz_track_id() const { return musicbrainz_track_id_; }
  const std::string &musicbrainz_disc_id() const { return musicbrainz_disc_id_; }
  const std::string &musicbrainz_release_group_id() const { return musicbrainz_release_group_id_; }
  const std::string &musicbrainz_work_id() const { return musicbrainz_work_id_; }

  std::optional<double> ebur128_integrated_loudness_lufs() const { return ebur128_integrated_loudness_lufs_; }
  std::optional<double> ebur128_loudness_range_lu() const { return ebur128_loudness_range_lu_; }

  void set_title(const std::string &v) { title_ = v; }
  void set_titlesort(const std::string &v) { titlesort_ = v; }
  void set_album(const std::string &v) { album_ = v; }
  void set_albumsort(const std::string &v) { albumsort_ = v; }
  void set_artist(const std::string &v) { artist_ = v; }
  void set_artistsort(const std::string &v) { artistsort_ = v; }
  void set_albumartist(const std::string &v) { albumartist_ = v; }
  void set_albumartistsort(const std::string &v) { albumartistsort_ = v; }
  void set_track(int v) { track_ = v; }
  void set_disc(int v) { disc_ = v; }
  void set_year(int v) { year_ = v; }
  void set_originalyear(int v) { originalyear_ = v; }
  void set_genre(const std::string &v) { genre_ = v; }
  void set_compilation(bool v) { compilation_ = v; }
  void set_composer(const std::string &v) { composer_ = v; }
  void set_composersort(const std::string &v) { composersort_ = v; }
  void set_performer(const std::string &v) { performer_ = v; }
  void set_performersort(const std::string &v) { performersort_ = v; }
  void set_grouping(const std::string &v) { grouping_ = v; }
  void set_comment(const std::string &v) { comment_ = v; }
  void set_lyrics(const std::string &v) { lyrics_ = v; }
  void set_artist_id(const std::string &v) { artist_id_ = v; }
  void set_album_id(const std::string &v) { album_id_ = v; }
  void set_song_id(const std::string &v) { song_id_ = v; }
  void set_beginning_nanosec(int64_t v) { beginning_nanosec_ = v; }
  void set_length_nanosec(int64_t v) { length_nanosec_ = v; }
  void set_bitrate(int v) { bitrate_ = v; }
  void set_samplerate(int v) { samplerate_ = v; }
  void set_bitdepth(int v) { bitdepth_ = v; }
  void set_source(Source v) { source_ = v; }
  void set_directory_id(int v) { directory_id_ = v; }
  void set_url(const std::string &v);
  void set_stream_url(const std::string &v) { stream_url_ = v; }
  void set_basefilename(const std::string &v) { basefilename_ = v; }
  void set_filetype(FileType v) { filetype_ = v; }
  void set_filesize(int64_t v) { filesize_ = v; }
  void set_mtime(int64_t v) { mtime_ = v; }
  void set_ctime(int64_t v) { ctime_ = v; }
  void set_unavailable(bool v) { unavailable_ = v; }
  void set_fingerprint(const std::string &v) { fingerprint_ = v; }
  void set_playcount(unsigned v) { playcount_ = v; }
  void set_skipcount(unsigned v) { skipcount_ = v; }
  void set_lastplayed(int64_t v) { lastplayed_ = v; }
  void set_lastseen(int64_t v) { lastseen_ = v; }
  void set_art_embedded(bool v) { art_embedded_ = v; }
  void set_art_automatic(const std::string &v) { art_automatic_ = v; }
  void set_art_manual(const std::string &v) { art_manual_ = v; }
  void set_art_unset(bool v) { art_unset_ = v; }
  void set_cue_path(const std::string &v) { cue_path_ = v; }
  void set_rating(float v) { rating_ = v; }
  void set_bpm(float v) { bpm_ = v; }
  void set_mood(const std::string &v) { mood_ = v; }
  void set_initial_key(const std::string &v) { initial_key_ = v; }
  void set_acoustid_id(const std::string &v) { acoustid_id_ = v; }
  void set_acoustid_fingerprint(const std::string &v) { acoustid_fingerprint_ = v; }
  void set_musicbrainz_album_artist_id(const std::string &v) { musicbrainz_album_artist_id_ = v; }
  void set_musicbrainz_artist_id(const std::string &v) { musicbrainz_artist_id_ = v; }
  void set_musicbrainz_original_artist_id(const std::string &v) { musicbrainz_original_artist_id_ = v; }
  void set_musicbrainz_album_id(const std::string &v) { musicbrainz_album_id_ = v; }
  void set_musicbrainz_original_album_id(const std::string &v) { musicbrainz_original_album_id_ = v; }
  void set_musicbrainz_recording_id(const std::string &v) { musicbrainz_recording_id_ = v; }
  void set_musicbrainz_track_id(const std::string &v) { musicbrainz_track_id_ = v; }
  void set_musicbrainz_disc_id(const std::string &v) { musicbrainz_disc_id_ = v; }
  void set_musicbrainz_release_group_id(const std::string &v) { musicbrainz_release_group_id_ = v; }
  void set_musicbrainz_work_id(const std::string &v) { musicbrainz_work_id_ = v; }
  void set_ebur128_integrated_loudness_lufs(std::optional<double> v) { ebur128_integrated_loudness_lufs_ = v; }
  void set_ebur128_loudness_range_lu(std::optional<double> v) { ebur128_loudness_range_lu_ = v; }

  std::string PrettyTitle() const;
  std::string PrettyTitleWithArtist() const;
  std::string EffectiveAlbumartist() const;
  bool is_stream() const;
  bool is_radio() const;
  bool is_stream_service() const;
  bool is_metadata_good() const;
  bool is_cdda() const { return source_ == Source::CDDA || filetype_ == FileType::CDDA; }
  bool is_collection_song() const { return source_ == Source::Collection; }
  bool is_local_file() const { return source_ == Source::LocalFile || source_ == Source::Collection; }
  bool IsEditable() const;

  int id3v2_version() const { return id3v2_version_; }
  void set_id3v2_version(int v) { id3v2_version_ = v; }
  bool id3v2_tags_supported() const {
    return filetype_ == FileType::MPEG || filetype_ == FileType::WAV || filetype_ == FileType::AIFF;
  }
  static bool save_embedded_cover_supported(FileType filetype) {
    return filetype == FileType::FLAC || filetype == FileType::OggVorbis || filetype == FileType::OggOpus ||
           filetype == FileType::MPEG || filetype == FileType::MP4 || filetype == FileType::WAV || filetype == FileType::AIFF;
  }
  bool save_embedded_cover_supported() const {
    return is_local_file() && save_embedded_cover_supported(filetype_) && cue_path_.empty();
  }

  static FileType FiletypeByExtension(const std::string &extension);
  static FileType FiletypeByMimeType(const std::string &mimetype);
  static FileType FiletypeByFilename(const std::string &filename);
  static bool IsAudioFile(const std::string &filename);
  static std::string SourceToString(Source source);
  static std::string FiletypeToString(FileType type);
  static std::string AlbumRemoveDiscMisc(const std::string &album);
  static const char *TextSearchColumnsSql();

 private:
  bool valid_ = false;
  int id_ = -1;
  std::string title_;
  std::string titlesort_;
  std::string album_;
  std::string albumsort_;
  std::string artist_;
  std::string artistsort_;
  std::string albumartist_;
  std::string albumartistsort_;
  int track_ = -1;
  int disc_ = -1;
  int year_ = -1;
  int originalyear_ = -1;
  std::string genre_;
  bool compilation_ = false;
  std::string composer_;
  std::string composersort_;
  std::string performer_;
  std::string performersort_;
  std::string grouping_;
  std::string comment_;
  std::string lyrics_;
  std::string artist_id_;
  std::string album_id_;
  std::string song_id_;
  int64_t beginning_nanosec_ = 0;
  int64_t length_nanosec_ = 0;
  int bitrate_ = -1;
  int samplerate_ = -1;
  int bitdepth_ = -1;
  Source source_ = Source::Unknown;
  int directory_id_ = -1;
  std::string url_;
  std::string stream_url_;
  std::string basefilename_;
  FileType filetype_ = FileType::Unknown;
  int64_t filesize_ = -1;
  int64_t mtime_ = -1;
  int64_t ctime_ = -1;
  bool unavailable_ = false;
  bool skipped_ = false;
  std::string fingerprint_;
  unsigned playcount_ = 0;
  unsigned skipcount_ = 0;
  int64_t lastplayed_ = -1;
  int64_t lastseen_ = -1;
  bool art_embedded_ = false;
  int id3v2_version_ = 0;
  std::string art_automatic_;
  std::string art_manual_;
  bool art_unset_ = false;
  std::string cue_path_;
  float rating_ = -1.0f;
  float bpm_ = -1.0f;
  std::string mood_;
  std::string initial_key_;
  std::string acoustid_id_;
  std::string acoustid_fingerprint_;
  std::string musicbrainz_album_artist_id_;
  std::string musicbrainz_artist_id_;
  std::string musicbrainz_original_artist_id_;
  std::string musicbrainz_album_id_;
  std::string musicbrainz_original_album_id_;
  std::string musicbrainz_recording_id_;
  std::string musicbrainz_track_id_;
  std::string musicbrainz_disc_id_;
  std::string musicbrainz_release_group_id_;
  std::string musicbrainz_work_id_;
  std::optional<double> ebur128_integrated_loudness_lufs_;
  std::optional<double> ebur128_loudness_range_lu_;
};

using SongList = std::vector<Song>;

#endif
