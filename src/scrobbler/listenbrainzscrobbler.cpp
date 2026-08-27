#include "scrobbler/listenbrainzscrobbler.h"

#include "constants/scrobblersettings.h"
#include "core/localredirectserver.h"
#include "core/oauthenticator.h"
#include "core/oauthpkce.h"
#include "core/settings.h"
#include "scrobbler/listenbrainzoauth.h"
#include "scrobbler/listenbrainzscrobblestate.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "utilities/randutils.h"
#include "utilities/strutils.h"
#include "version.h"

#include <gio/gio.h>
#include <glib.h>

#include <ctime>

const char *ListenBrainzScrobbler::kSubmitUrl = "https://api.listenbrainz.org/1/submit-listens";
const char *ListenBrainzScrobbler::kFeedbackUrl = "https://api.listenbrainz.org/1/feedback/recording-feedback";
const char *ListenBrainzScrobbler::kCacheFile = "listenbrainzscrobbler.cache";

namespace {

struct ListenBrainzOAuthRedirect {
  ListenBrainzScrobbler *self = nullptr;
  std::string code;
  std::string redirect_uri;
  std::string verifier;
  std::function<void(bool)> done;
};

bool OfflineMode() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  return settings.BoolValue(ScrobblerSettings::kOffline, ScrobblerSettings::kDefaultOffline);
}

Song SongFromMetadata(const Song &song, const ScrobbleMetadata &metadata) {
  Song processed = song;
  processed.set_artist(metadata.artist);
  processed.set_album(metadata.album);
  processed.set_title(metadata.title);
  processed.set_albumartist(metadata.albumartist);
  return processed;
}

}  // namespace

ListenBrainzScrobbler::ListenBrainzScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile), song_playing_(Song()) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  token_ = settings.Value("token");
  username_ = settings.Value("username");
  access_token_ = settings.Value("access_token");
}

ListenBrainzScrobbler::~ListenBrainzScrobbler() {
  CancelOAuthIdle();
  CloseRedirectServer();
  CancelSubmitTimer();
}

void ListenBrainzScrobbler::CloseRedirectServer() { redirect_server_.reset(); }

void ListenBrainzScrobbler::CancelOAuthIdle() {
  if (oauth_idle_id_ != 0) {
    g_source_remove(oauth_idle_id_);
    oauth_idle_id_ = 0;
  }
}

void ListenBrainzScrobbler::SaveAccessToken() const {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("access_token", access_token_);
  settings.Sync();
}

void ListenBrainzScrobbler::CancelSubmitTimer() {
  if (submit_timeout_id_ != 0) {
    g_source_remove(submit_timeout_id_);
    submit_timeout_id_ = 0;
  }
}

std::string ListenBrainzScrobbler::SubmitBody(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items) {
  std::string body = "{\"listen_type\":\"" + StrUtils::JsonEscape(listen_type) + "\",\"payload\":[";
  for (size_t i = 0; i < items.size(); ++i) {
    if (i) {
      body += ",";
    }
    body += "{";
    if (listen_type != "playing_now" && items[i].timestamp > 0) {
      body += "\"listened_at\":" + std::to_string(items[i].timestamp) + ",";
    }
    body += "\"track_metadata\":{\"artist_name\":\"" + StrUtils::JsonEscape(items[i].artist) + "\",\"track_name\":\"" +
            StrUtils::JsonEscape(items[i].title) + "\"";
    if (ListenBrainzScrobbleState::ShouldIncludeReleaseName(items[i].album)) {
      body += ",\"release_name\":\"" + StrUtils::JsonEscape(items[i].album) + "\"";
    }
    body += ",\"additional_info\":" + ListenBrainzScrobbleState::AdditionalInfoJson(items[i], STRAWBERRY_VERSION_DISPLAY) + "}}";
  }
  body += "]}";
  return body;
}

std::string ListenBrainzScrobbler::LoveBody(const std::string &recording_mbid) {
  return std::string("{\"recording_mbid\":\"") + StrUtils::JsonEscape(recording_mbid) + "\",\"score\":1}";
}

void ListenBrainzScrobbler::Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache) {
  if (!enabled_ || !network_ || items.empty() || OfflineMode()) {
    submitted_ = false;
    if (from_cache) {
      cache_.ClearSent();
    }
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    submitted_ = false;
    if (from_cache) {
      cache_.ClearSent();
    }
    Error.Emit(ScrobblerError::NotAuthenticated("ListenBrainz"));
    return;
  }
  network_->Post(kSubmitUrl, SubmitBody(listen_type, items), [this, from_cache](const NetworkAccessManager::Response &response) {
    submitted_ = false;
    if (from_cache && response.ok()) {
      cache_.RemoveSent();
      submit_error_ = false;
    } else if (!response.ok()) {
      Error.Emit(ScrobblerError::RequestFailed("ListenBrainz"));
      if (from_cache) {
        cache_.ClearSent();
        ScheduleSubmit(true);
      }
    }
  }, "application/json", {{"Authorization", "Token " + token}});
}

void ListenBrainzScrobbler::CheckScrobblePrevSong() {
  const uint64_t now = static_cast<uint64_t>(std::time(nullptr));
  if (!ScrobblerPlayingState::ShouldScrobbleRadioPrev(song_playing_, scrobbled_,
                                                      ScrobblerPlayingState::ElapsedSeconds(timestamp_, now))) {
    return;
  }
  Song song = song_playing_;
  song.set_length_nanosec(ScrobblerPlayingState::ElapsedSeconds(timestamp_, now) * 1000000000LL);
  Scrobble(song);
}

void ListenBrainzScrobbler::NowPlaying(const Song &song) {
  CheckScrobblePrevSong();
  song_playing_ = song;
  timestamp_ = static_cast<uint64_t>(std::time(nullptr));
  scrobbled_ = false;
  if (!song.is_metadata_good()) {
    return;
  }
  const ScrobbleMetadata metadata = ScrobbleMetadata::FromSongSettings(song);
  ScrobblerCacheItem item;
  item.artist = metadata.artist;
  item.album = metadata.album;
  item.title = metadata.title;
  item.track = metadata.track;
  item.length_nanosec = metadata.length_nanosec;
  item.musicbrainz_album_artist_id = metadata.musicbrainz_album_artist_id;
  item.musicbrainz_artist_id = metadata.musicbrainz_artist_id;
  item.musicbrainz_original_artist_id = metadata.musicbrainz_original_artist_id;
  item.musicbrainz_album_id = metadata.musicbrainz_album_id;
  item.musicbrainz_original_album_id = metadata.musicbrainz_original_album_id;
  item.musicbrainz_recording_id = metadata.musicbrainz_recording_id;
  item.musicbrainz_track_id = metadata.musicbrainz_track_id;
  item.musicbrainz_disc_id = metadata.musicbrainz_disc_id;
  item.musicbrainz_release_group_id = metadata.musicbrainz_release_group_id;
  item.musicbrainz_work_id = metadata.musicbrainz_work_id;
  item.music_service = metadata.music_service;
  item.music_service_name = metadata.music_service_name;
  item.share_url = metadata.share_url;
  item.spotify_id = metadata.spotify_id;
  Submit("playing_now", {item}, false);
}

void ListenBrainzScrobbler::ClearPlaying() {
  CheckScrobblePrevSong();
  song_playing_ = Song();
  scrobbled_ = false;
  timestamp_ = 0;
}

void ListenBrainzScrobbler::Scrobble(const Song &song) {
  if (!ScrobblerPlayingState::SameAsPlaying(song, song_playing_)) {
    return;
  }
  scrobbled_ = true;
  cache_.Add(SongFromMetadata(song, ScrobbleMetadata::FromSongSettings(song)),
             ScrobblerPlayingState::TimestampOrNow(timestamp_, static_cast<uint64_t>(std::time(nullptr))));
  if (OfflineMode() || token_.empty()) {
    return;
  }
  ScheduleSubmit(false);
}

void ListenBrainzScrobbler::WriteCache() { cache_.Save(); }

void ListenBrainzScrobbler::ScheduleSubmit(bool had_error) {
  submit_error_ = had_error;
  if (!ListenBrainzScrobbleState::ShouldStartSubmitTimer(submitted_, !cache_.Unsent().empty(), submit_timeout_id_ != 0)) {
    return;
  }
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const int delay = ListenBrainzScrobbleState::DelaySeconds(settings.IntValue(ScrobblerSettings::kSubmit, ScrobblerSettings::kDefaultSubmit),
                                                            had_error);
  CancelSubmitTimer();
  submit_timeout_id_ = g_timeout_add_seconds(static_cast<guint>(delay), +[](gpointer data) -> gboolean {
                                               auto *self = static_cast<ListenBrainzScrobbler *>(data);
                                               self->submit_timeout_id_ = 0;
                                               self->FlushCache();
                                               return G_SOURCE_REMOVE;
                                             },
                                             this);
}

void ListenBrainzScrobbler::FlushCache() {
  if (submitted_) {
    return;
  }
  const std::vector<ScrobblerCacheItem> items = cache_.Unsent();
  if (items.empty()) {
    return;
  }
  cache_.MarkSent();
  submitted_ = true;
  Submit(ListenBrainzScrobbleState::CachedListenType(), items, true);
}

void ListenBrainzScrobbler::Submit() {
  CancelSubmitTimer();
  FlushCache();
}

void ListenBrainzScrobbler::Love(const Song &song) {
  if (!enabled_ || !network_ || song.musicbrainz_recording_id().empty() || OfflineMode()) {
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    return;
  }
  network_->Post(kFeedbackUrl, LoveBody(song.musicbrainz_recording_id()), [](const NetworkAccessManager::Response &) {}, "application/json",
                 {{"Authorization", "Token " + token}});
}

void ListenBrainzScrobbler::Authenticate(const std::string &username, const std::string &token) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("username", username);
  settings.SetValue("token", token);
  settings.Sync();
  username_ = username;
  token_ = token;
}

void ListenBrainzScrobbler::StartAuthorization(NetworkAccessManager *network, std::function<void(bool)> done) {
  if (!ListenBrainzOAuth::ShouldStartAuthorization(network)) {
    if (done) {
      done(false);
    }
    return;
  }
  CloseRedirectServer();
  redirect_server_ = std::make_unique<LocalRedirectServer>();
  if (!redirect_server_->Listen()) {
    CloseRedirectServer();
    if (done) {
      done(false);
    }
    return;
  }
  const std::string redirect_uri = ListenBrainzOAuth::RedirectUri(redirect_server_->port());
  const std::string verifier = RandUtils::CryptographicRandomString(OAuthPkce::kVerifierLength);
  const std::string challenge = OAuthPkce::ChallengeS256(verifier);
  const std::string url = ListenBrainzOAuth::AuthorizationUrl(redirect_uri, challenge);
  redirect_server_->Redirected.Connect([this, redirect_uri, verifier, done](const std::string &returned) {
    auto *state = new ListenBrainzOAuthRedirect;
    state->self = this;
    state->code = ListenBrainzOAuth::ExtractCode(returned);
    state->redirect_uri = redirect_uri;
    state->verifier = verifier;
    state->done = done;
    oauth_idle_id_ = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, +[](gpointer data) -> gboolean {
      auto *owned = static_cast<ListenBrainzOAuthRedirect *>(data);
      owned->self->oauth_idle_id_ = 0;
      owned->self->CloseRedirectServer();
      if (owned->code.empty()) {
        if (owned->done) {
          owned->done(false);
        }
      } else {
        owned->self->ExchangeAuthorizationCode(owned->code, owned->redirect_uri, owned->verifier, owned->done);
      }
      return G_SOURCE_REMOVE;
    }, state, [](gpointer data) { delete static_cast<ListenBrainzOAuthRedirect *>(data); });
  });
  if (!g_app_info_launch_default_for_uri(url.c_str(), nullptr, nullptr)) {
    CloseRedirectServer();
    if (done) {
      done(false);
    }
  }
}

void ListenBrainzScrobbler::ExchangeAuthorizationCode(const std::string &code, const std::string &redirect_uri, const std::string &verifier,
                                                     std::function<void(bool)> done) {
  if (!network_) {
    if (done) {
      done(false);
    }
    return;
  }
  auto *oauth = new OAuthenticator(network_);
  oauth->set_redirect_uri(redirect_uri);
  oauth->ExchangeCode(ListenBrainzOAuth::kTokenUrl, ListenBrainzOAuth::ClientId(), ListenBrainzOAuth::ClientSecret(), code,
                      [this, oauth, done](const std::string &body, const std::string &error) {
                        const auto tokens = OAuthenticator::ParseTokenResponse(body);
                        const bool ok = error.empty() && !tokens.access_token.empty();
                        if (ok) {
                          access_token_ = tokens.access_token;
                          SaveAccessToken();
                        }
                        delete oauth;
                        if (done) {
                          done(ok);
                        }
                      },
                      verifier);
}

void ListenBrainzScrobbler::Logout() {
  token_.clear();
  username_.clear();
  access_token_.clear();
  CloseRedirectServer();
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("username", "");
  settings.SetValue("token", "");
  settings.SetValue("access_token", "");
  settings.Sync();
}
