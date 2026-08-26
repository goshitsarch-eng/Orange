#include "mpris2/mpris2.h"

#include "config.h"
#include "core/application.h"
#include "core/logging.h"
#include "core/player.h"
#include "mpris2/mpris2helpers.h"
#include "playlist/playlistmanager.h"

#include <algorithm>

#ifdef HAVE_MPRIS2
static const gchar *kMprisXml =
    "<node>"
    "  <interface name='org.mpris.MediaPlayer2'>"
    "    <method name='Raise'/>"
    "    <method name='Quit'/>"
    "    <property name='CanQuit' type='b' access='read'/>"
    "    <property name='CanRaise' type='b' access='read'/>"
    "    <property name='HasTrackList' type='b' access='read'/>"
    "    <property name='Identity' type='s' access='read'/>"
    "    <property name='DesktopEntry' type='s' access='read'/>"
    "    <property name='SupportedUriSchemes' type='as' access='read'/>"
    "    <property name='SupportedMimeTypes' type='as' access='read'/>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2.Player'>"
    "    <method name='Next'/>"
    "    <method name='Previous'/>"
    "    <method name='Pause'/>"
    "    <method name='PlayPause'/>"
    "    <method name='Stop'/>"
    "    <method name='Play'/>"
    "    <method name='Seek'><arg type='x' name='Offset'/></method>"
    "    <method name='SetPosition'><arg type='o' name='TrackId'/><arg type='x' name='Position'/></method>"
    "    <method name='OpenUri'><arg type='s' name='Uri'/></method>"
    "    <property name='PlaybackStatus' type='s' access='read'/>"
    "    <property name='LoopStatus' type='s' access='readwrite'/>"
    "    <property name='Rate' type='d' access='readwrite'/>"
    "    <property name='Shuffle' type='b' access='readwrite'/>"
    "    <property name='Metadata' type='a{sv}' access='read'/>"
    "    <property name='Volume' type='d' access='readwrite'/>"
    "    <property name='Position' type='x' access='read'/>"
    "    <property name='CanGoNext' type='b' access='read'/>"
    "    <property name='CanGoPrevious' type='b' access='read'/>"
    "    <property name='CanPlay' type='b' access='read'/>"
    "    <property name='CanPause' type='b' access='read'/>"
    "    <property name='CanSeek' type='b' access='read'/>"
    "    <property name='CanControl' type='b' access='read'/>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2.TrackList'>"
    "    <method name='GetTracksMetadata'>"
    "      <arg type='ao' name='TrackIds' direction='in'/>"
    "      <arg type='aa{sv}' name='Metadata' direction='out'/>"
    "    </method>"
    "    <method name='AddTrack'>"
    "      <arg type='s' name='Uri' direction='in'/>"
    "      <arg type='o' name='AfterTrack' direction='in'/>"
    "      <arg type='b' name='SetAsCurrent' direction='in'/>"
    "    </method>"
    "    <method name='RemoveTrack'>"
    "      <arg type='o' name='TrackId' direction='in'/>"
    "    </method>"
    "    <method name='GoTo'>"
    "      <arg type='o' name='TrackId' direction='in'/>"
    "    </method>"
    "    <property name='Tracks' type='ao' access='read'/>"
    "    <property name='CanEditTracks' type='b' access='read'/>"
    "    <signal name='TrackListReplaced'>"
    "      <arg type='ao' name='Tracks'/>"
    "      <arg type='o' name='CurrentTrack'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static GVariant *MetadataVariant(const Song &song, int row = -1) {
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  const std::string track_id = row >= 0 ? Mpris2Helpers::TrackIdForRow(song, row) : Mpris2Helpers::TrackId(song);
  g_variant_builder_add(&b, "{sv}", "mpris:trackid", g_variant_new_object_path(track_id.c_str()));
  g_variant_builder_add(&b, "{sv}", "xesam:title", g_variant_new_string(song.title().c_str()));
  g_variant_builder_add(&b, "{sv}", "xesam:album", g_variant_new_string(song.album().c_str()));
  const gchar *artists[] = {song.artist().c_str(), nullptr};
  g_variant_builder_add(&b, "{sv}", "xesam:artist", g_variant_new_strv(artists, -1));
  g_variant_builder_add(&b, "{sv}", "xesam:url", g_variant_new_string(song.url().c_str()));
  if (song.length_nanosec() > 0) {
    g_variant_builder_add(&b, "{sv}", "mpris:length", g_variant_new_int64(song.length_nanosec() / 1000));
  }
  const std::string art = Mpris2Helpers::ArtUrl(song);
  if (!art.empty()) {
    g_variant_builder_add(&b, "{sv}", "mpris:artUrl", g_variant_new_string(art.c_str()));
  }
  if (song.track() > 0) {
    g_variant_builder_add(&b, "{sv}", "xesam:trackNumber", g_variant_new_int32(song.track()));
  }
  if (!song.albumartist().empty()) {
    const gchar *albumartists[] = {song.albumartist().c_str(), nullptr};
    g_variant_builder_add(&b, "{sv}", "xesam:albumArtist", g_variant_new_strv(albumartists, -1));
  }
  return g_variant_builder_end(&b);
}

static int TrackListRow(Playlist *playlist, const char *track_id) {
  if (!playlist || !track_id) {
    return -1;
  }
  for (int i = 0; i < playlist->row_count(); ++i) {
    if (Mpris2Helpers::TrackIdForRow(playlist->song(i), i) == track_id || Mpris2Helpers::TrackId(playlist->song(i)) == track_id) {
      return i;
    }
  }
  return -1;
}

static GVariant *TrackListIds(Playlist *playlist) {
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("ao"));
  if (playlist) {
    for (int i = 0; i < playlist->row_count(); ++i) {
      const std::string id = Mpris2Helpers::TrackIdForRow(playlist->song(i), i);
      g_variant_builder_add(&b, "o", id.c_str());
    }
  }
  return g_variant_builder_end(&b);
}

static void HandleMethod(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *method, GVariant *parameters,
                         GDBusMethodInvocation *invocation, gpointer user_data) {
  auto *self = static_cast<Mpris2 *>(user_data);
  Application *app = self ? self->app() : nullptr;
  if (app) {
    if (g_strcmp0(method, "Raise") == 0) {
      app->RaiseRequested.Emit();
    } else if (g_strcmp0(method, "Quit") == 0) {
      app->Exit();
    } else if (app->player()) {
      if (g_strcmp0(method, "Play") == 0) {
        app->player()->Play();
      } else if (g_strcmp0(method, "Pause") == 0) {
        app->player()->Pause();
      } else if (g_strcmp0(method, "PlayPause") == 0) {
        app->player()->PlayPause();
      } else if (g_strcmp0(method, "Stop") == 0) {
        app->player()->Stop();
      } else if (g_strcmp0(method, "Next") == 0) {
        app->player()->Next();
      } else if (g_strcmp0(method, "Previous") == 0) {
        app->player()->Previous();
      } else if (g_strcmp0(method, "Seek") == 0) {
        gint64 offset = 0;
        g_variant_get(parameters, "(x)", &offset);
        app->player()->SeekTo(app->player()->engine()->position_nanosec() / 1000000000LL + offset / 1000000);
        if (self) {
          self->EmitSeeked(app->player()->engine()->position_nanosec() / 1000);
        }
      } else if (g_strcmp0(method, "SetPosition") == 0) {
        const gchar *track_id = nullptr;
        gint64 position = 0;
        g_variant_get(parameters, "(&ox)", &track_id, &position);
        app->player()->Seek(position * 1000);
        if (self) {
          self->EmitSeeked(position);
        }
      } else if (g_strcmp0(method, "OpenUri") == 0) {
        const gchar *uri = nullptr;
        g_variant_get(parameters, "(&s)", &uri);
        if (uri && app->playlist_manager()) {
          app->playlist_manager()->InsertUrls({uri});
          app->player()->Play();
        }
      } else if (g_strcmp0(method, "GetTracksMetadata") == 0) {
        GVariantIter *iter = nullptr;
        g_variant_get(parameters, "(ao)", &iter);
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));
        Playlist *playlist = app->playlist_manager() ? app->playlist_manager()->current() : nullptr;
        const gchar *id = nullptr;
        while (iter && g_variant_iter_loop(iter, "o", &id)) {
          const int row = TrackListRow(playlist, id);
          if (row >= 0) {
            g_variant_builder_add_value(&b, MetadataVariant(playlist->song(row), row));
          }
        }
        if (iter) {
          g_variant_iter_free(iter);
        }
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(aa{sv})", &b));
        return;
      } else if (g_strcmp0(method, "AddTrack") == 0) {
        const gchar *uri = nullptr;
        const gchar *after = nullptr;
        gboolean current = FALSE;
        g_variant_get(parameters, "(&s&ob)", &uri, &after, &current);
        if (uri && app->playlist_manager()) {
          Playlist *playlist = app->playlist_manager()->current();
          int insert_row = -1;
          if (after && std::string(after) != "/org/mpris/MediaPlayer2/TrackList/NoTrack") {
            const int after_row = TrackListRow(playlist, after);
            if (after_row >= 0) {
              insert_row = after_row + 1;
            }
          }
          app->playlist_manager()->InsertUrls({uri}, insert_row);
          if (current) {
            app->player()->Play();
          }
          if (self) {
            self->EmitTrackListReplaced();
          }
        }
      } else if (g_strcmp0(method, "RemoveTrack") == 0) {
        const gchar *id = nullptr;
        g_variant_get(parameters, "(&o)", &id);
        Playlist *playlist = app->playlist_manager() ? app->playlist_manager()->current() : nullptr;
        const int row = TrackListRow(playlist, id);
        if (playlist && row >= 0) {
          playlist->RemoveRows({row});
          if (self) {
            self->EmitTrackListReplaced();
          }
        }
      } else if (g_strcmp0(method, "GoTo") == 0) {
        const gchar *id = nullptr;
        g_variant_get(parameters, "(&o)", &id);
        Playlist *playlist = app->playlist_manager() ? app->playlist_manager()->current() : nullptr;
        const int row = TrackListRow(playlist, id);
        if (row >= 0) {
          app->player()->PlayAt(row);
        }
      }
    }
  }
  g_dbus_method_invocation_return_value(invocation, nullptr);
}

static GVariant *HandleGet(GDBusConnection *, const gchar *, const gchar *, const gchar *interface, const gchar *property, GError **,
                           gpointer user_data) {
  auto *self = static_cast<Mpris2 *>(user_data);
  Application *app = self ? self->app() : nullptr;
  if (g_strcmp0(property, "Identity") == 0) {
    return g_variant_new_string("Strawberry");
  }
  if (g_strcmp0(property, "DesktopEntry") == 0) {
    return g_variant_new_string("org.strawberrymusicplayer.strawberry");
  }
  if (g_strcmp0(property, "CanQuit") == 0 || g_strcmp0(property, "CanRaise") == 0 || g_strcmp0(property, "CanPlay") == 0 ||
      g_strcmp0(property, "CanPause") == 0 || g_strcmp0(property, "CanGoNext") == 0 || g_strcmp0(property, "CanGoPrevious") == 0 ||
      g_strcmp0(property, "CanSeek") == 0 || g_strcmp0(property, "CanControl") == 0) {
    return g_variant_new_boolean(TRUE);
  }
  if (g_strcmp0(property, "HasTrackList") == 0) {
    return g_variant_new_boolean(TRUE);
  }
  if (g_strcmp0(property, "CanEditTracks") == 0) {
    return g_variant_new_boolean(TRUE);
  }
  if (g_strcmp0(property, "Tracks") == 0) {
    Playlist *playlist = app && app->playlist_manager() ? app->playlist_manager()->current() : nullptr;
    return TrackListIds(playlist);
  }
  if (g_strcmp0(property, "PlaybackStatus") == 0) {
    if (!app || !app->player()) {
      return g_variant_new_string("Stopped");
    }
    switch (app->player()->GetState()) {
      case GstEngine::State::Playing:
        return g_variant_new_string("Playing");
      case GstEngine::State::Paused:
        return g_variant_new_string("Paused");
      default:
        return g_variant_new_string("Stopped");
    }
  }
  if (g_strcmp0(property, "Rate") == 0) {
    return g_variant_new_double(1.0);
  }
  if (g_strcmp0(property, "Volume") == 0) {
    return g_variant_new_double(app && app->player() ? app->player()->GetVolume() / 100.0 : 1.0);
  }
  if (g_strcmp0(property, "Position") == 0) {
    const int64_t pos = app && app->player() ? app->player()->engine()->position_nanosec() / 1000 : 0;
    return g_variant_new_int64(pos);
  }
  if (g_strcmp0(property, "LoopStatus") == 0) {
    PlaylistSequence::RepeatMode mode = PlaylistSequence::RepeatMode::Off;
    if (app && app->playlist_manager() && app->playlist_manager()->active()) {
      mode = app->playlist_manager()->active()->repeat_mode();
    }
    return g_variant_new_string(Mpris2Helpers::LoopStatus(mode).c_str());
  }
  if (g_strcmp0(property, "Shuffle") == 0) {
    bool shuffle = false;
    if (app && app->playlist_manager() && app->playlist_manager()->active()) {
      shuffle = app->playlist_manager()->active()->shuffle_mode() != PlaylistSequence::ShuffleMode::Off;
    }
    return g_variant_new_boolean(shuffle);
  }
  if (g_strcmp0(property, "SupportedUriSchemes") == 0) {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    for (const char *scheme : {"file", "http", "https", "cdda", "smb", "ftp"}) {
      g_variant_builder_add(&b, "s", scheme);
    }
    return g_variant_builder_end(&b);
  }
  if (g_strcmp0(property, "SupportedMimeTypes") == 0) {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    for (const char *mime : {"audio/mpeg", "audio/flac", "audio/ogg", "audio/x-wav", "audio/mp4", "audio/x-ms-wma"}) {
      g_variant_builder_add(&b, "s", mime);
    }
    return g_variant_builder_end(&b);
  }
  if (g_strcmp0(property, "Metadata") == 0) {
    Song song;
    int row = -1;
    if (app && app->player()) {
      song = app->player()->current_song();
    }
    if (app && app->playlist_manager() && app->playlist_manager()->active()) {
      row = app->playlist_manager()->active()->current_row();
    }
    return MetadataVariant(song, row);
  }
  (void)interface;
  return nullptr;
}

static gboolean HandleSet(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *property, GVariant *value, GError **,
                          gpointer user_data) {
  auto *self = static_cast<Mpris2 *>(user_data);
  Application *app = self ? self->app() : nullptr;
  if (!app) {
    return TRUE;
  }
  if (g_strcmp0(property, "Volume") == 0 && app->player()) {
    app->player()->SetVolume(static_cast<unsigned>(std::clamp(g_variant_get_double(value) * 100.0, 0.0, 100.0)));
  } else if (g_strcmp0(property, "Shuffle") == 0 && app->playlist_manager() && app->playlist_manager()->active()) {
    app->playlist_manager()->active()->SetShuffleMode(g_variant_get_boolean(value) ? PlaylistSequence::ShuffleMode::All
                                                                                   : PlaylistSequence::ShuffleMode::Off);
  } else if (g_strcmp0(property, "LoopStatus") == 0 && app->playlist_manager() && app->playlist_manager()->active()) {
    const gchar *status = g_variant_get_string(value, nullptr);
    app->playlist_manager()->active()->SetRepeatMode(Mpris2Helpers::RepeatFromLoopStatus(status ? status : "None"));
  }
  return TRUE;
}

static const GDBusInterfaceVTable kVtable = {HandleMethod, HandleGet, HandleSet, {nullptr}};
#endif

Mpris2::Mpris2(Application *app) : app_(app) {
#ifdef HAVE_MPRIS2
  if (app_ && app_->player()) {
    app_->player()->StateChanged.Connect([this](EngineBase::State) { EmitPlaybackStatus(); });
    app_->player()->SongChanged.Connect([this](const Song &) { EmitMetadata(); });
    app_->player()->VolumeChanged.Connect([this](unsigned) { EmitVolume(); });
  }
  if (app_ && app_->playlist_manager()) {
    app_->playlist_manager()->CurrentChanged.Connect([this](Playlist *) { EmitTrackListReplaced(); });
  }
  owner_id_ = g_bus_own_name(G_BUS_TYPE_SESSION, "org.mpris.MediaPlayer2.strawberry", G_BUS_NAME_OWNER_FLAGS_NONE, OnBusAcquired, nullptr,
                             nullptr, this, nullptr);
#endif
}

Mpris2::~Mpris2() {
#ifdef HAVE_MPRIS2
  if (owner_id_) {
    g_bus_unown_name(owner_id_);
  }
#endif
}

void Mpris2::OnBusAcquired(GDBusConnection *connection, const gchar *, gpointer user_data) {
#ifdef HAVE_MPRIS2
  auto *self = static_cast<Mpris2 *>(user_data);
  self->connection_ = connection;
  GError *error = nullptr;
  GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(kMprisXml, &error);
  if (!info) {
    if (error) {
      g_error_free(error);
    }
    return;
  }
  g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2", info->interfaces[0], &kVtable, self, nullptr, nullptr);
  g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2", info->interfaces[1], &kVtable, self, nullptr, nullptr);
  if (info->interfaces[2]) {
    g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2", info->interfaces[2], &kVtable, self, nullptr, nullptr);
  }
  g_dbus_node_info_unref(info);
#else
  (void)connection;
  (void)user_data;
#endif
}

void Mpris2::EmitPropertiesChanged(const char *interface_name, const char *property, GVariant *value) {
#ifdef HAVE_MPRIS2
  if (!connection_ || !value) {
    if (value) {
      g_variant_unref(g_variant_ref_sink(value));
    }
    return;
  }
  GVariantBuilder changed;
  g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&changed, "{sv}", property, value);
  GVariantBuilder invalidated;
  g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));
  g_dbus_connection_emit_signal(connection_, nullptr, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                g_variant_new("(sa{sv}as)", interface_name, &changed, &invalidated), nullptr);
#else
  (void)interface_name;
  (void)property;
  (void)value;
#endif
}

void Mpris2::EmitSeeked(int64_t position_us) {
#ifdef HAVE_MPRIS2
  if (!connection_) {
    return;
  }
  g_dbus_connection_emit_signal(connection_, nullptr, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Seeked",
                                g_variant_new("(x)", position_us), nullptr);
#else
  (void)position_us;
#endif
}

void Mpris2::EmitPlaybackStatus() {
#ifdef HAVE_MPRIS2
  const char *status = "Stopped";
  if (app_ && app_->player()) {
    switch (app_->player()->GetState()) {
      case GstEngine::State::Playing:
        status = "Playing";
        break;
      case GstEngine::State::Paused:
        status = "Paused";
        break;
      default:
        break;
    }
  }
  EmitPropertiesChanged("org.mpris.MediaPlayer2.Player", "PlaybackStatus", g_variant_new_string(status));
#endif
}

void Mpris2::EmitMetadata() {
#ifdef HAVE_MPRIS2
  Song song;
  int row = -1;
  if (app_ && app_->player()) {
    song = app_->player()->current_song();
  }
  if (app_ && app_->playlist_manager() && app_->playlist_manager()->active()) {
    row = app_->playlist_manager()->active()->current_row();
  }
  EmitPropertiesChanged("org.mpris.MediaPlayer2.Player", "Metadata", MetadataVariant(song, row));
#endif
}

void Mpris2::EmitVolume() {
#ifdef HAVE_MPRIS2
  const double volume = app_ && app_->player() ? app_->player()->GetVolume() / 100.0 : 1.0;
  EmitPropertiesChanged("org.mpris.MediaPlayer2.Player", "Volume", g_variant_new_double(volume));
#endif
}

void Mpris2::EmitTrackListReplaced() {
#ifdef HAVE_MPRIS2
  if (!connection_) {
    return;
  }
  Playlist *playlist = app_ && app_->playlist_manager() ? app_->playlist_manager()->current() : nullptr;
  std::string current = "/org/mpris/MediaPlayer2/TrackList/NoTrack";
  if (playlist && playlist->current_row() >= 0) {
    current = Mpris2Helpers::TrackIdForRow(playlist->current_song(), playlist->current_row());
  }
  g_dbus_connection_emit_signal(connection_, nullptr, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.TrackList", "TrackListReplaced",
                                g_variant_new("(@aoo)", TrackListIds(playlist), current.c_str()), nullptr);
  EmitPropertiesChanged("org.mpris.MediaPlayer2.TrackList", "Tracks", TrackListIds(playlist));
#endif
}
