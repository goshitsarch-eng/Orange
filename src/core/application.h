#ifndef STRAWBERRY_APPLICATION_H
#define STRAWBERRY_APPLICATION_H

#include "analyzer/analyzer.h"
#include "collection/collectionlibrary.h"
#include "covermanager/albumcoverloader.h"
#include "covermanager/coverproviders.h"
#include "covermanager/currentalbumcoverloader.h"
#include "core/commandlineoptions.h"
#include "core/database.h"
#include "core/network.h"
#include "core/player.h"
#include "core/signal.h"
#include "core/taskmanager.h"
#include "core/urlhandlers.h"
#include "device/devicemanager.h"
#include "discord/discord.h"
#include "engine/devicefinders.h"
#include "equalizer/equalizer.h"
#include "globalshortcuts/globalshortcuts.h"
#include "lyrics/lyricsfetcher.h"
#include "lyrics/lyricsproviders.h"
#include "moodbar/moodbar.h"
#include "mpris2/mpris2.h"
#include "organize/organize.h"
#include "osd/osd.h"
#include "playlist/playlistbackend.h"
#include "playlist/playlistmanager.h"
#include "queue/queue.h"
#include "radios/radioservices.h"
#include "scrobbler/audioscrobbler.h"
#include "streaming/streamingservices.h"
#include "systemtrayicon/systemtrayicon.h"
#include "tagfetcher/tagfetcher.h"
#include "tagreader/tagreader.h"
#include "tagreader/tagreaderclient.h"
#include "transcoder/transcoder.h"
#include "waveform/waveform.h"

#include <memory>

class Application {
 public:
  Application();
  ~Application();

  void Init();
  void Exit();
  bool WaitingForExitFade() const { return waiting_for_fade_ && !exit_started_; }
  void ApplyCommandline(const CommandlineOptions &options);

  TaskManager *task_manager() const { return task_manager_.get(); }
  Database *database() const { return database_.get(); }
  NetworkAccessManager *network() const { return network_.get(); }
  TagReader *tagreader() const { return tagreader_.get(); }
  TagReaderClient *tagreader_client() const { return tagreader_client_.get(); }
  UrlHandlers *url_handlers() const { return url_handlers_.get(); }
  CollectionLibrary *collection() const { return collection_.get(); }
  PlaylistBackend *playlist_backend() const { return playlist_backend_.get(); }
  PlaylistManager *playlist_manager() const { return playlist_manager_.get(); }
  Player *player() const { return player_.get(); }
  Queue *queue() const;
  CoverProviders *cover_providers() const { return cover_providers_.get(); }
  AlbumCoverLoader *albumcover_loader() const { return albumcover_loader_.get(); }
  CurrentAlbumCoverLoader *current_albumcover_loader() const { return current_albumcover_loader_.get(); }
  LyricsProviders *lyrics_providers() const { return lyrics_providers_.get(); }
  LyricsFetcher *lyrics_fetcher() const { return lyrics_fetcher_.get(); }
  AudioScrobbler *scrobbler() const { return scrobbler_.get(); }
  StreamingServices *streaming_services() const { return streaming_services_.get(); }
  RadioServices *radio_services() const { return radio_services_.get(); }
  DeviceManager *device_manager() const { return device_manager_.get(); }
  DeviceFinders *device_finders() const { return device_finders_.get(); }
  Equalizer *equalizer() const { return equalizer_.get(); }
  Analyzer *analyzer() const { return analyzer_.get(); }
  MoodbarController *moodbar() const { return moodbar_.get(); }
  WaveformController *waveform() const { return waveform_.get(); }
  Transcoder *transcoder() const { return transcoder_.get(); }
  OSD *osd() const { return osd_.get(); }
  SystemTrayIcon *tray() const { return tray_.get(); }
  Mpris2 *mpris() const { return mpris_.get(); }
  GlobalShortcutsManager *shortcuts() const { return shortcuts_.get(); }
  DiscordRichPresence *discord() const { return discord_.get(); }
  TagFetcher *tag_fetcher() const { return tag_fetcher_.get(); }

  Signal<> ExitFinished;
  Signal<> RaiseRequested;
  Signal<> HideForExit;

 private:
  void WatchPlaylistOsd(class Playlist *playlist);
  void BindPlayerQueue();
  std::unique_ptr<TaskManager> task_manager_;
  std::unique_ptr<Database> database_;
  std::unique_ptr<NetworkAccessManager> network_;
  std::unique_ptr<TagReader> tagreader_;
  std::unique_ptr<TagReaderClient> tagreader_client_;
  std::unique_ptr<UrlHandlers> url_handlers_;
  std::unique_ptr<CollectionLibrary> collection_;
  std::unique_ptr<PlaylistBackend> playlist_backend_;
  std::unique_ptr<PlaylistManager> playlist_manager_;
  std::unique_ptr<Player> player_;
  std::unique_ptr<Queue> queue_;
  std::unique_ptr<CoverProviders> cover_providers_;
  std::unique_ptr<AlbumCoverLoader> albumcover_loader_;
  std::unique_ptr<CurrentAlbumCoverLoader> current_albumcover_loader_;
  std::unique_ptr<LyricsProviders> lyrics_providers_;
  std::unique_ptr<LyricsFetcher> lyrics_fetcher_;
  std::unique_ptr<AudioScrobbler> scrobbler_;
  std::unique_ptr<StreamingServices> streaming_services_;
  std::unique_ptr<RadioServices> radio_services_;
  std::unique_ptr<DeviceManager> device_manager_;
  std::unique_ptr<DeviceFinders> device_finders_;
  std::unique_ptr<Equalizer> equalizer_;
  std::unique_ptr<Analyzer> analyzer_;
  std::unique_ptr<MoodbarLoader> moodbar_loader_;
  std::unique_ptr<MoodbarController> moodbar_;
  std::unique_ptr<WaveformLoader> waveform_loader_;
  std::unique_ptr<WaveformController> waveform_;
  std::unique_ptr<Transcoder> transcoder_;
  std::unique_ptr<OSD> osd_;
  std::unique_ptr<SystemTrayIcon> tray_;
  std::unique_ptr<Mpris2> mpris_;
  std::unique_ptr<GlobalShortcutsManager> shortcuts_;
  std::unique_ptr<DiscordRichPresence> discord_;
  std::unique_ptr<TagFetcher> tag_fetcher_;
  bool playback_was_paused_ = false;
  int playlist_osd_gen_ = 0;
  int exit_count_ = 0;
  bool exit_started_ = false;
  bool waiting_for_fade_ = false;

  void CompleteExit();
};

#endif
