#include "analyzer/analyzer.h"
#include "constants/analyzersettings.h"
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
#include "settings/settingscontrols.h"
#include "covermanager/coverproviderauth.h"
#include "settings/settingspages.h"
#include "streaming/streamingchoices.h"

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

TEST(SettingsControls, NormalizationAndScales) {
  EXPECT_STREQ("none", SettingsControls::NormalizationChoice(false, false));
  EXPECT_STREQ("rg", SettingsControls::NormalizationChoice(true, false));
  EXPECT_STREQ("ebu", SettingsControls::NormalizationChoice(false, true));
  EXPECT_STREQ("ebu", SettingsControls::NormalizationChoice(true, true));
  EXPECT_TRUE(SettingsControls::NormalizationUsesReplayGain("rg"));
  EXPECT_FALSE(SettingsControls::NormalizationUsesReplayGain("none"));
  EXPECT_TRUE(SettingsControls::NormalizationUsesEbu("ebu"));
  EXPECT_EQ(0.01, SettingsControls::ApplyRange(-1.0, SettingsControls::BufferWatermark()));
  EXPECT_EQ(1.0, SettingsControls::ApplyRange(2.0, SettingsControls::BufferWatermark()));
  EXPECT_EQ(4000.0, SettingsControls::ApplyRange(4000.0, SettingsControls::BufferDurationMs()));
  EXPECT_EQ(-15.0, SettingsControls::ApplyRange(-40.0, SettingsControls::ReplayGainDb()));
  EXPECT_EQ(-23.0, SettingsControls::ApplyRange(-23.0, SettingsControls::EbuTargetLufs()));
  EXPECT_TRUE(SettingsControls::PlaylistColorIsSystem({}));
  EXPECT_FALSE(SettingsControls::PlaylistColorIsSystem("#6696e3"));
  EXPECT_TRUE(SettingsControls::PlaylistPlayingSongColor(true, "#6696e3").empty());
  EXPECT_EQ("#6696e3", SettingsControls::PlaylistPlayingSongColor(false, {}));
  EXPECT_EQ("#abcabc", SettingsControls::PlaylistPlayingSongColor(false, "#abcabc"));
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
  EXPECT_STREQ("quality", TidalSettings::kQuality);
  EXPECT_STREQ("streamurl", TidalSettings::kStreamUrl);
  EXPECT_STREQ("LOSSLESS", TidalSettings::kDefaultQuality);
  EXPECT_STREQ("app_id", QobuzSettings::kAppId);
  EXPECT_STREQ("app_secret", QobuzSettings::kAppSecret);
  EXPECT_STREQ("format", QobuzSettings::kFormat);
  EXPECT_EQ(27, QobuzSettings::kDefaultFormat);
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

TEST(AnalyzerSettings, KeysDefaultsAndTypes) {
  EXPECT_STREQ("Analyzer", AnalyzerSettings::kSettingsGroup);
  EXPECT_STREQ("type", AnalyzerSettings::kType);
  EXPECT_STREQ("enabled", AnalyzerSettings::kEnabled);
  EXPECT_STREQ("framerate", AnalyzerSettings::kFramerate);
  EXPECT_STREQ("Bar", AnalyzerSettings::kDefaultType);
  EXPECT_TRUE(AnalyzerSettings::kDefaultEnabled);
  EXPECT_EQ(25, AnalyzerSettings::kDefaultFramerate);
  const auto types = Analyzer::Types();
  EXPECT_EQ("Bar", types.front());
  EXPECT_EQ("Block", types.back());
  EXPECT_EQ(8u, types.size());
}

TEST(SettingsPages, ServicePageNames) {
  EXPECT_STREQ("Tidal", SettingsPages::ForService("Tidal"));
  EXPECT_STREQ("Qobuz", SettingsPages::ForService("Qobuz"));
  EXPECT_STREQ("Spotify", SettingsPages::ForService("Spotify"));
  EXPECT_STREQ("Subsonic", SettingsPages::ForService("Subsonic"));
  EXPECT_EQ(nullptr, SettingsPages::ForService("Unknown"));
  EXPECT_STREQ("Tidal", SettingsPages::ForSource(Song::Source::Tidal));
  EXPECT_STREQ("Qobuz", SettingsPages::ForSource(Song::Source::Qobuz));
  EXPECT_STREQ("Spotify", SettingsPages::ForSource(Song::Source::Spotify));
  EXPECT_STREQ("Subsonic", SettingsPages::ForSource(Song::Source::Subsonic));
  EXPECT_STREQ("Collection", SettingsPages::ForSource(Song::Source::Collection));
  EXPECT_STREQ("Radio", SettingsPages::ForSource(Song::Source::RadioBrowser));
  EXPECT_EQ(nullptr, SettingsPages::ForSource(Song::Source::LocalFile));
  EXPECT_TRUE(SettingsPages::CanOpenAt(SettingsPages::Tidal()));
  EXPECT_TRUE(SettingsPages::CanOpenAt(SettingsPages::Collection()));
  EXPECT_STREQ("Configure collection…", SettingsPages::ConfigureCollectionLabel());
  EXPECT_FALSE(SettingsPages::CanOpenAt(nullptr));
  EXPECT_FALSE(SettingsPages::CanOpenAt(""));
}

TEST(CoverProviderAuth, StatusTextAndServiceSettings) {
  EXPECT_EQ(3u, CoverProviderAuth::ServiceSettingsProviders().size());
  EXPECT_EQ(CoverProviderAuth::Mode::ServiceSettings, CoverProviderAuth::ModeFor("Tidal"));
  EXPECT_EQ(CoverProviderAuth::Mode::ServiceSettings, CoverProviderAuth::ModeFor("Spotify"));
  EXPECT_EQ(CoverProviderAuth::Mode::ServiceSettings, CoverProviderAuth::ModeFor("Qobuz"));
  EXPECT_EQ(CoverProviderAuth::Mode::None, CoverProviderAuth::ModeFor("Last.fm"));
  EXPECT_FALSE(CoverProviderAuth::RequiresAuthentication("MusicBrainz"));
  EXPECT_EQ("Last.fm does not need authentication.", CoverProviderAuth::StatusText("Last.fm", false));
  EXPECT_EQ("Use Tidal settings to authenticate.", CoverProviderAuth::StatusText("Tidal", false));
  EXPECT_EQ("Tidal needs authentication.", CoverProviderAuth::StatusText("Tidal", true));
  EXPECT_EQ("Use Spotify settings to authenticate.", CoverProviderAuth::StatusText("Spotify", false));
  EXPECT_EQ("Use Qobuz settings to authenticate.", CoverProviderAuth::StatusText("Qobuz", false));
  EXPECT_EQ("No cover provider selected.", CoverProviderAuth::StatusText({}, false));
  EXPECT_STREQ("Tidal", CoverProviderAuth::SettingsPageName("Tidal"));
  EXPECT_STREQ("Spotify", CoverProviderAuth::SettingsPageName("Spotify"));
  EXPECT_STREQ("Qobuz", CoverProviderAuth::SettingsPageName("Qobuz"));
  EXPECT_EQ(nullptr, CoverProviderAuth::SettingsPageName("Last.fm"));
  EXPECT_TRUE(CoverProviderAuth::ShowOpenSettings("Tidal"));
  EXPECT_FALSE(CoverProviderAuth::ShowOpenSettings("Last.fm"));
  EXPECT_EQ("Open Tidal settings", CoverProviderAuth::OpenSettingsLabel("Tidal"));
}

TEST(StreamingChoices, QualityAndAuthLabels) {
  EXPECT_EQ("LOSSLESS", StreamingChoices::TidalQualities()[2].first);
  EXPECT_EQ("2", StreamingChoices::TidalStreamUrlMethods().back().first);
  EXPECT_EQ("27", StreamingChoices::QobuzFormats().back().first);
  EXPECT_EQ("1", StreamingChoices::SubsonicAuthMethods().back().first);
  EXPECT_EQ("highest", StreamingChoices::SomaFmQualities().front().first);
  EXPECT_EQ("aac-320", StreamingChoices::RadioParadiseStreams().front().first);
}
