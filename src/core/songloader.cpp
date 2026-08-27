#include "core/songloader.h"

#include "collection/collectionbackend.h"
#include "core/commandlineurl.h"
#include "core/loadurl.h"
#include "core/network.h"
#include "core/songloadremote.h"
#include "core/songloadsort.h"
#include "core/songloadtypefind.h"
#include "core/urlhandlers.h"
#include "device/cddasongloader.h"
#include "playlistparsers/parserbase.h"
#include "playlistparsers/playlistparser.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib-object.h>
#include <gst/gst.h>

#include <mutex>

namespace {

struct RemoteTypefindState {
  std::mutex mutex;
  std::string mime;
  std::string data;
  bool have_type = false;
  bool eos = false;
  bool failed = false;
  bool type_not_found = false;
};

}  // namespace

SongLoader::SongLoader(UrlHandlers *url_handlers, CollectionBackend *collection_backend, TagReader *tagreader)
    : url_handlers_(url_handlers), collection_backend_(collection_backend), tagreader_(tagreader) {}

SongLoader::Result SongLoader::Load(const std::string &url) {
  if (url.empty()) {
    errors_.push_back("Empty URL");
    return Result::Error;
  }
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(url)) {
      const UrlHandler::LoadResult loaded = handler->Load(url);
      if (loaded.type == UrlHandler::LoadResult::Type::TrackAvailable) {
        Song song = loaded.song;
        if (!song.is_valid()) {
          song.set_url(loaded.media_url.empty() ? url : loaded.media_url);
          song.set_stream_url(loaded.stream_url);
          song.set_valid(true);
        }
        songs_.push_back(song);
        return Result::Success;
      }
      if (loaded.type == UrlHandler::LoadResult::Type::Error) {
        errors_.push_back(loaded.error.empty() ? url : loaded.error);
        return Result::Error;
      }
    }
  }
  if (CommandlineUrl::IsLocalFile(url)) {
    return LoadLocal(CommandlineUrl::LocalPath(url));
  }
  if (SongLoadRemote::ShouldAddAsRawStream(url)) {
    AddRawStream(url);
    return Result::Success;
  }
  pending_remote_urls_.push_back(url);
  return Result::BlockingLoadRequired;
}

SongLoader::Result SongLoader::LoadMany(const std::vector<std::string> &urls) {
  bool blocking = false;
  bool error = false;
  for (const std::string &url : urls) {
    const Result result = Load(url);
    if (result == Result::BlockingLoadRequired) {
      blocking = true;
    }
    if (result == Result::Error) {
      error = true;
    }
  }
  if (blocking || !pending_paths_.empty() || !pending_remote_urls_.empty()) {
    return Result::BlockingLoadRequired;
  }
  if (songs_.empty()) {
    return error ? Result::Error : Result::Error;
  }
  return Result::Success;
}

SongLoader::Result SongLoader::LoadFilenamesBlocking() {
  const std::vector<std::string> queued = pending_paths_;
  pending_paths_.clear();
  for (const std::string &path : queued) {
    if (FileUtils::IsDirectory(path)) {
      LoadLocalDirectory(path);
    } else {
      LoadLocal(path);
    }
  }
  const std::vector<std::string> remotes = pending_remote_urls_;
  pending_remote_urls_.clear();
  for (const std::string &url : remotes) {
    LoadRemote(url);
  }
  return songs_.empty() ? Result::Error : Result::Success;
}

void SongLoader::LoadMetadataBlocking() {
  for (Song &song : songs_) {
    EffectiveSongLoad(&song);
  }
}

SongLoader::Result SongLoader::LoadRemote(const std::string &url) {
  if (SongLoadRemote::LooksLikePlaylist(url)) {
    NetworkAccessManager network;
    constexpr guint kRemoteTimeoutSec = 5;
    g_object_set(network.session(), "timeout", kRemoteTimeoutSec, nullptr);
    const NetworkAccessManager::Response response = network.GetSync(url);
    if (!response.ok()) {
      errors_.push_back(response.error.empty() ? ("Failed to retrieve " + url) : response.error);
      return Result::Error;
    }
    return LoadRemoteFromData(url, response.body);
  }
  return TypefindRemote(url);
}

SongLoader::Result SongLoader::ApplyRemoteTypefind(const std::string &url, const std::string &mime, const std::string &data) {
  PlaylistParser parser;
  ParserBase *found = parser.ParserForMagic(data);
  const SongLoadTypefind::Decision decision = SongLoadTypefind::Decide(mime, found != nullptr);
  if (decision == SongLoadTypefind::Decision::RawStream) {
    AddRawStream(url);
    return Result::Success;
  }
  if (decision == SongLoadTypefind::Decision::Parse && found) {
    SongList loaded = found->Load(data, url);
    if (!loaded.empty()) {
      if (playlist_name_.empty()) {
        playlist_name_ = FileUtils::BaseName(SongLoadRemote::PathWithoutQuery(url));
      }
      songs_.insert(songs_.end(), loaded.begin(), loaded.end());
      return Result::Success;
    }
  }
  errors_.push_back("Failed to identify " + url);
  return Result::Error;
}

SongLoader::Result SongLoader::TypefindRemote(const std::string &url) {
  if (!gst_is_initialized()) {
    GError *init_error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &init_error)) {
      if (init_error) {
        errors_.push_back(init_error->message);
        g_error_free(init_error);
      }
      AddRawStream(url);
      return Result::Success;
    }
  }

  GError *uri_error = nullptr;
  GstElement *source = gst_element_make_from_uri(GST_URI_SRC, url.c_str(), nullptr, &uri_error);
  if (!source) {
    if (uri_error) {
      g_error_free(uri_error);
    }
    // No protocol handler (this environment has no souphttpsrc): play as a stream instead of downloading.
    AddRawStream(url);
    return Result::Success;
  }

  GstElement *pipeline = gst_pipeline_new("songloader-typefind");
  GstElement *typefind = gst_element_factory_make("typefind", nullptr);
  GstElement *fakesink = gst_element_factory_make("fakesink", nullptr);
  if (!pipeline || !typefind || !fakesink) {
    if (source) {
      gst_object_unref(source);
    }
    if (pipeline) {
      gst_object_unref(pipeline);
    }
    errors_.push_back("Couldn't create GStreamer typefind pipeline for " + url);
    return Result::Error;
  }

  if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "ssl-strict")) {
    g_object_set(source, "ssl-strict", FALSE, nullptr);
  }

  gst_bin_add_many(GST_BIN(pipeline), source, typefind, fakesink, nullptr);
  if (!gst_element_link_many(source, typefind, fakesink, nullptr)) {
    gst_object_unref(pipeline);
    errors_.push_back("Couldn't link GStreamer typefind pipeline for " + url);
    return Result::Error;
  }

  RemoteTypefindState state;
  g_signal_connect(typefind, "have-type",
                   G_CALLBACK((+[](GstElement *, guint, GstCaps *caps, gpointer user) {
                     auto *st = static_cast<RemoteTypefindState *>(user);
                     if (!caps || gst_caps_get_size(caps) == 0) {
                       return;
                     }
                     const GstStructure *structure = gst_caps_get_structure(caps, 0);
                     const gchar *name = structure ? gst_structure_get_name(structure) : nullptr;
                     std::lock_guard<std::mutex> lock(st->mutex);
                     st->mime = name ? name : "";
                     st->have_type = true;
                   })),
                   &state);

  GstPad *pad = gst_element_get_static_pad(fakesink, "sink");
  if (pad) {
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER,
                      +[](GstPad *, GstPadProbeInfo *info, gpointer user) -> GstPadProbeReturn {
                        auto *st = static_cast<RemoteTypefindState *>(user);
                        GstBuffer *buffer = gst_pad_probe_info_get_buffer(info);
                        GstMapInfo map;
                        if (buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
                          std::lock_guard<std::mutex> lock(st->mutex);
                          st->data.append(reinterpret_cast<const char *>(map.data), static_cast<std::size_t>(map.size));
                          gst_buffer_unmap(buffer, &map);
                        }
                        return GST_PAD_PROBE_OK;
                      },
                      &state, nullptr);
    gst_object_unref(pad);
  }

  GstBus *bus = gst_element_get_bus(pipeline);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  const gint64 deadline = g_get_monotonic_time() + 5 * G_USEC_PER_SEC;
  while (g_get_monotonic_time() < deadline) {
    GstMessage *msg = bus ? gst_bus_timed_pop(bus, 50 * GST_MSECOND) : nullptr;
    if (msg) {
      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
          GError *err = nullptr;
          gst_message_parse_error(msg, &err, nullptr);
          if (err && err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_TYPE_NOT_FOUND) {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.type_not_found = true;
          } else {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.failed = true;
          }
          if (err) {
            g_error_free(err);
          }
          break;
        }
        case GST_MESSAGE_EOS: {
          std::lock_guard<std::mutex> lock(state.mutex);
          state.eos = true;
          break;
        }
        default:
          break;
      }
      gst_message_unref(msg);
    }
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.failed) {
      break;
    }
    if (state.have_type && !state.mime.empty() && !SongLoadTypefind::MimeMightBePlaylist(state.mime)) {
      break;
    }
    if ((state.have_type || state.type_not_found) && (state.mime.empty() || SongLoadTypefind::MimeMightBePlaylist(state.mime)) &&
        state.data.size() >= SongLoadTypefind::kMagicSize) {
      break;
    }
    if (state.eos) {
      break;
    }
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  if (bus) {
    gst_object_unref(bus);
  }
  gst_object_unref(pipeline);

  std::string mime;
  std::string data;
  bool failed = false;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    mime = state.mime;
    data = state.data;
    failed = state.failed;
  }
  if (failed) {
    errors_.push_back("Failed to identify " + url);
    return Result::Error;
  }
  return ApplyRemoteTypefind(url, mime, data);
}

SongLoader::Result SongLoader::LoadRemoteFromData(const std::string &url, const std::string &data) {
  PlaylistParser parser;
  ParserBase *found = parser.ParserForExtension(SongLoadRemote::Extension(url));
  if (!found) {
    found = parser.ParserForMagic(data);
  }
  SongList loaded;
  if (found) {
    loaded = found->Load(data, url);
  }
  if (!loaded.empty()) {
    if (playlist_name_.empty()) {
      playlist_name_ = FileUtils::BaseName(SongLoadRemote::PathWithoutQuery(url));
    }
    songs_.insert(songs_.end(), loaded.begin(), loaded.end());
    return Result::Success;
  }
  if (SongLoadRemote::LooksLikePlaylist(url)) {
    errors_.push_back("Failed to parse playlist: " + url);
    return Result::Error;
  }
  AddRawStream(url);
  return Result::Success;
}

SongLoader::Result SongLoader::LoadAudioCD() {
  const SongList songs = CddaSongLoader().LoadDevice({});
  if (songs.empty()) {
    errors_.push_back("No audio CD found");
    return Result::Error;
  }
  songs_.insert(songs_.end(), songs.begin(), songs.end());
  playlist_name_ = "Audio CD";
  return Result::Success;
}

SongLoader::Result SongLoader::LoadLocal(const std::string &path) {
  if (path.empty() || !FileUtils::Exists(path)) {
    errors_.push_back(LoadUrl::MissingFileMessage(path.empty() ? "URL" : path));
    return Result::Error;
  }
  if (FileUtils::IsDirectory(path)) {
    pending_paths_.push_back(path);
    return Result::BlockingLoadRequired;
  }
  if (PlaylistParser::IsPlaylist(path)) {
    LoadPlaylistFile(path);
    return songs_.empty() ? Result::Error : Result::Success;
  }
  if (Song::IsAudioFile(path)) {
    LoadAudioFile(path);
    return Result::Success;
  }
  errors_.push_back("Unsupported file: " + path);
  return Result::Error;
}

void SongLoader::LoadLocalDirectory(const std::string &path) {
  for (const std::string &entry : FileUtils::ListDirectoryRecursive(path)) {
    if (PlaylistParser::IsPlaylist(entry) || Song::IsAudioFile(entry)) {
      LoadLocal(entry);
    }
  }
  SongLoadSort::StableSort(songs_);
  if (!songs_.empty()) {
    EffectiveSongLoad(&songs_.front());
  }
}

void SongLoader::LoadPlaylistFile(const std::string &path) {
  playlist_name_ = FileUtils::BaseName(path);
  PlaylistParser parser;
  SongList loaded = parser.Load(path);
  if (StrUtils::ToLower(FileUtils::Extension(path)) == "cue" && !loaded.empty() && tagreader_) {
    const std::string audio = FileUtils::PathFromUri(loaded.front().url());
    if (FileUtils::IsFile(audio)) {
      PlaylistParser::EnrichFromAudioFile(&loaded, tagreader_->ReadFile(audio));
    }
  }
  songs_.insert(songs_.end(), loaded.begin(), loaded.end());
}

void SongLoader::LoadAudioFile(const std::string &path) {
  const std::string cue = PlaylistParser::FindCueForAudio(path);
  if (!cue.empty()) {
    LoadPlaylistFile(cue);
    return;
  }
  if (collection_backend_) {
    Song collection = collection_backend_->SongByUrl(FileUtils::UriFromPath(path));
    if (collection.is_valid()) {
      songs_.push_back(collection);
      return;
    }
  }
  Song song;
  if (tagreader_) {
    song = tagreader_->ReadFile(path);
  }
  if (!song.is_valid()) {
    song.set_url(FileUtils::UriFromPath(path));
    song.set_title(FileUtils::BaseName(path));
    song.set_valid(true);
  }
  songs_.push_back(song);
}

void SongLoader::AddRawStream(const std::string &url) {
  Song song(Song::Source::Stream);
  song.set_url(url);
  song.set_title(url);
  song.set_valid(true);
  songs_.push_back(song);
}

void SongLoader::EffectiveSongLoad(Song *song) {
  if (!song || !tagreader_) {
    return;
  }
  const std::string path = FileUtils::PathFromUri(song->url());
  if (!FileUtils::IsFile(path)) {
    return;
  }
  Song loaded = tagreader_->ReadFile(path);
  if (loaded.is_valid()) {
    *song = loaded;
  }
}
