#include "constants/backendsettings.h"
#include "constants/behavioursettings.h"
#include "constants/collectionsettings.h"
#include "constants/contextsettings.h"
#include "constants/coverssettings.h"
#include "constants/globalshortcutssettings.h"
#include "constants/moodbarsettings.h"
#include "constants/networkproxysettings.h"
#include "constants/notificationssettings.h"
#include "constants/playlistsettings.h"
#include "constants/qobuzsettings.h"
#include "constants/radiobrowsersettings.h"
#include "constants/scrobblersettings.h"
#include "constants/somafmsettings.h"
#include "constants/spotifysettings.h"
#include "constants/subsonicsettings.h"
#include "constants/tidalsettings.h"
#include "constants/transcodersettings.h"
#include "constants/waveformsettings.h"

#include <gtest/gtest.h>

TEST(BehaviourSettings, OriginalKeysAndDefaults) {
  EXPECT_STREQ("Behaviour", BehaviourSettings::kSettingsGroup);
  EXPECT_STREQ("keeprunning", BehaviourSettings::kKeepRunning);
  EXPECT_STREQ("showtrayicon", BehaviourSettings::kShowTrayIcon);
  EXPECT_STREQ("resumeplayback", BehaviourSettings::kResumePlayback);
  EXPECT_STREQ("seek_step_sec", BehaviourSettings::kSeekStepSec);
  EXPECT_STREQ("volume_increment", BehaviourSettings::kVolumeIncrement);
  EXPECT_FALSE(BehaviourSettings::kDefaultKeepRunning);
  EXPECT_TRUE(BehaviourSettings::kDefaultShowTrayIcon);
  EXPECT_FALSE(BehaviourSettings::kDefaultResumePlayback);
  EXPECT_EQ(10, BehaviourSettings::kDefaultSeekStepSec);
  EXPECT_EQ(5u, BehaviourSettings::kDefaultVolumeIncrement);
}

TEST(CollectionSettings, OriginalKeysAndDefaults) {
  EXPECT_STREQ("startup_scan", CollectionSettings::kStartupScan);
  EXPECT_STREQ("pretty_covers", CollectionSettings::kPrettyCovers);
  EXPECT_STREQ("various_artists", CollectionSettings::kVariousArtists);
  EXPECT_STREQ("skip_articles_for_artists", CollectionSettings::kSkipArticlesForArtists);
  EXPECT_STREQ("song_tracking", CollectionSettings::kSongTracking);
  EXPECT_TRUE(CollectionSettings::kDefaultStartupScan);
  EXPECT_TRUE(CollectionSettings::kDefaultPrettyCovers);
  EXPECT_TRUE(CollectionSettings::kDefaultSkipArticlesForArtists);
  EXPECT_EQ(60, CollectionSettings::kDefaultExpireUnavailableSongs);
}

TEST(BackendSettings, OriginalFadeAndReplayGainKeys) {
  EXPECT_STREQ("rgenabled", BackendSettings::kRgEnabled);
  EXPECT_STREQ("FadeoutEnabled", BackendSettings::kFadeoutEnabled);
  EXPECT_STREQ("CrossfadeEnabled", BackendSettings::kCrossfadeEnabled);
  EXPECT_STREQ("AutoCrossfadeEnabled", BackendSettings::kAutoCrossfadeEnabled);
  EXPECT_STREQ("FadeoutDuration", BackendSettings::kFadeoutDuration);
  EXPECT_STREQ("ebur128_loudness_normalization", BackendSettings::kEBUR128LoudnessNormalization);
  EXPECT_EQ(2000, BackendSettings::kDefaultFadeoutDuration);
  EXPECT_FALSE(BackendSettings::kDefaultRgEnabled);
}

TEST(PlaylistSettings, OriginalBehaviourKeys) {
  EXPECT_STREQ("alternating_row_colors", PlaylistSettings::kAlternatingRowColors);
  EXPECT_STREQ("show_bars", PlaylistSettings::kShowBars);
  EXPECT_STREQ("warn_close_playlist", PlaylistSettings::kWarnClosePlaylist);
  EXPECT_STREQ("continue_on_error", PlaylistSettings::kContinueOnError);
  EXPECT_STREQ("write_metadata", PlaylistSettings::kWriteMetadata);
  EXPECT_TRUE(PlaylistSettings::kDefaultWriteMetadata);
  EXPECT_FALSE(PlaylistSettings::kDefaultContinueOnError);
}

TEST(StreamingAndOsdSettings, OriginalGroups) {
  EXPECT_STREQ("OSD", OSDSettings::kSettingsGroup);
  EXPECT_STREQ("Behaviour", OSDSettings::kType);
  EXPECT_STREQ("ShowArt", OSDSettings::kShowArt);
  EXPECT_STREQ("Timeout", OSDSettings::kTimeout);
  EXPECT_STREQ("OSDPretty", OSDPrettySettings::kSettingsGroup);
  EXPECT_STREQ("DiscordRPC", DiscordRPCSettings::kSettingsGroup);
  EXPECT_STREQ("client_id", TidalSettings::kClientId);
  EXPECT_STREQ("app_id", QobuzSettings::kAppId);
  EXPECT_STREQ("user_auth_token", QobuzSettings::kUserAuthToken);
  EXPECT_STREQ("access_token", SpotifySettings::kAccessToken);
  EXPECT_STREQ("authmethod", SubsonicSettings::kAuthMethod);
  EXPECT_STREQ("hide_broken", RadioBrowserSettings::kHideBroken);
  EXPECT_STREQ("quality", SomaFMSettings::kQuality);
  EXPECT_STREQ("providers", CoversSettings::kProviders);
  EXPECT_STREQ("AlbumEnable", ContextSettings::kAlbum);
  EXPECT_STREQ("use_kglobalaccel", GlobalShortcutsSettings::kUseKGlobalAccel);
  EXPECT_STREQ("style", MoodbarSettings::kStyle);
  EXPECT_STREQ("color", WaveformSettings::kColor);
  EXPECT_EQ(8080u, NetworkProxySettings::kDefaultPort);
  EXPECT_STREQ("audio/x-vorbis", TranscoderSettings::kDefaultLastOutputFormat);
}
