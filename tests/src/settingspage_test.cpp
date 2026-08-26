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
#include "core/appearancestyle.h"
#include "settings/backendoutputchoices.h"
#include "settings/behaviourstartupchoices.h"
#include "settings/networkproxylabels.h"
#include "settings/settingscontrols.h"
#include "settings/streamingsettingslabels.h"
#include "subsonic/subsonicping.h"
#include "subsonic/subsonicsettingsactions.h"
#include "context/contextfontcontrols.h"
#include "context/contextfontpreview.h"
#include "widgets/loginstatevisibility.h"
#include "settings/transcodersettingspage.h"
#include "covermanager/coverproviderauth.h"
#include "settings/settingspages.h"
#include "streaming/streamingchoices.h"

#include <gtest/gtest.h>
#include <limits>

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
  EXPECT_FALSE(SettingsControls::FadeDurationEnabled(false, false, false));
  EXPECT_TRUE(SettingsControls::FadeDurationEnabled(true, false, false));
  EXPECT_TRUE(SettingsControls::FadeDurationEnabled(false, true, false));
  EXPECT_TRUE(SettingsControls::FadeDurationEnabled(false, false, true));
  EXPECT_FALSE(SettingsControls::PauseFadeEnabled(false));
  EXPECT_TRUE(SettingsControls::PauseFadeEnabled(true));
  EXPECT_FALSE(SettingsControls::ChannelsSpinEnabled(false));
  EXPECT_TRUE(SettingsControls::ChannelsSpinEnabled(true));
}

TEST(AppearanceStyle, GtkThemeChoicesAndCss) {
  const auto choices = AppearanceStyle::Choices();
  ASSERT_EQ(4u, choices.size());
  EXPECT_EQ("", choices.front().first);
  EXPECT_EQ("Adwaita Dark", choices[2].second);
  EXPECT_TRUE(AppearanceStyle::ForcesDark(AppearanceStyle::kAdwaitaDark));
  EXPECT_FALSE(AppearanceStyle::ForcesDark(AppearanceStyle::kAdwaita));
  EXPECT_TRUE(AppearanceStyle::HasCustomPalette(""));
  EXPECT_FALSE(AppearanceStyle::HasCustomPalette(AppearanceStyle::kHighContrast));
  EXPECT_TRUE(AppearanceStyle::CssFor(AppearanceStyle::kAdwaita).empty());
  EXPECT_FALSE(AppearanceStyle::CssFor(AppearanceStyle::kHighContrast).empty());
}

TEST(SettingsControls, BufferDefaultsAndCacheUnitMax) {
  const SettingsControls::BufferValues defaults = SettingsControls::BufferDefaults();
  EXPECT_EQ(4000, defaults.duration_ms);
  EXPECT_DOUBLE_EQ(0.33, defaults.low_watermark);
  EXPECT_DOUBLE_EQ(0.99, defaults.high_watermark);
  EXPECT_EQ(500, defaults.warmup_ms);
  EXPECT_EQ(std::numeric_limits<int>::max() / 1024,
            SettingsControls::IconCacheSizeMax(static_cast<int>(CollectionSettings::CacheSizeUnit::MB)));
  EXPECT_EQ(std::numeric_limits<int>::max(),
            SettingsControls::IconCacheSizeMax(static_cast<int>(CollectionSettings::CacheSizeUnit::KB)));
  EXPECT_EQ(4, SettingsControls::DiskCacheSizeMax(static_cast<int>(CollectionSettings::CacheSizeUnit::GB)));
  EXPECT_EQ(std::numeric_limits<int>::max(),
            SettingsControls::DiskCacheSizeMax(static_cast<int>(CollectionSettings::CacheSizeUnit::MB)));
  EXPECT_EQ(4, SettingsControls::ClampCacheSize(16, 4));
  EXPECT_EQ(0, SettingsControls::ClampCacheSize(-3, 4));
  EXPECT_EQ(2, SettingsControls::ClampCacheSize(2, 4));
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

TEST(NetworkProxyLabels, MatchQtRadiosAndTypeOrder) {
  EXPECT_STREQ("Network Proxy", NetworkProxyLabels::PageTitle());
  EXPECT_STREQ("Use the system proxy settings", NetworkProxyLabels::SystemMode());
  EXPECT_STREQ("Direct internet connection", NetworkProxyLabels::DirectMode());
  EXPECT_STREQ("Manual proxy configuration", NetworkProxyLabels::ManualMode());
  EXPECT_STREQ("HTTP proxy", NetworkProxyLabels::HttpType());
  EXPECT_STREQ("SOCKS proxy", NetworkProxyLabels::SocksType());
  EXPECT_STREQ("Use authentication", NetworkProxyLabels::AuthTitle());
  EXPECT_STREQ("Use proxy settings for streaming", NetworkProxyLabels::EngineLabel());
  EXPECT_STREQ("Only HTTP proxy is supported for streaming.", NetworkProxyLabels::EngineTooltip());
  EXPECT_FALSE(NetworkProxyLabels::ManualEnabled(0));
  EXPECT_FALSE(NetworkProxyLabels::ManualEnabled(1));
  EXPECT_TRUE(NetworkProxyLabels::ManualEnabled(2));
  EXPECT_TRUE(NetworkProxyLabels::ManualEnabled(std::string("2")));
  EXPECT_FALSE(NetworkProxyLabels::ManualEnabled(std::string("0")));
  const auto modes = NetworkProxyLabels::ModeChoices();
  ASSERT_EQ(3u, modes.size());
  EXPECT_EQ("0", modes[0].first);
  EXPECT_EQ("2", modes[2].first);
  const auto types = NetworkProxyLabels::TypeChoices();
  ASSERT_EQ(2u, types.size());
  EXPECT_EQ(std::to_string(static_cast<int>(NetworkProxySettings::ProxyType::HttpProxy)), types[0].first);
  EXPECT_EQ(std::to_string(static_cast<int>(NetworkProxySettings::ProxyType::Socks5Proxy)), types[1].first);
  EXPECT_EQ(0, NetworkProxyLabels::ComboIndexFromType(NetworkProxySettings::ProxyType::HttpProxy));
  EXPECT_EQ(1, NetworkProxyLabels::ComboIndexFromType(NetworkProxySettings::ProxyType::Socks5Proxy));
  EXPECT_EQ(NetworkProxySettings::ProxyType::HttpProxy, NetworkProxyLabels::TypeFromComboIndex(0));
  EXPECT_EQ(NetworkProxySettings::ProxyType::Socks5Proxy, NetworkProxyLabels::TypeFromComboIndex(1));
}

TEST(TranscoderSettingsPage, TabsMatchQtOrderAndGroups) {
  const std::vector<Transcoder::Format> tabs = TranscoderSettingsPage::TabFormats();
  ASSERT_EQ(8u, tabs.size());
  EXPECT_EQ(Transcoder::Format::FLAC, tabs[0]);
  EXPECT_EQ(Transcoder::Format::WavPack, tabs[1]);
  EXPECT_EQ(Transcoder::Format::OggVorbis, tabs[2]);
  EXPECT_EQ(Transcoder::Format::Opus, tabs[3]);
  EXPECT_EQ(Transcoder::Format::Speex, tabs[4]);
  EXPECT_EQ(Transcoder::Format::AAC, tabs[5]);
  EXPECT_EQ(Transcoder::Format::ASF, tabs[6]);
  EXPECT_EQ(Transcoder::Format::MP3, tabs[7]);
  EXPECT_STREQ("FLAC", TranscoderSettingsPage::TabLabel(Transcoder::Format::FLAC));
  EXPECT_STREQ("Vorbis", TranscoderSettingsPage::TabLabel(Transcoder::Format::OggVorbis));
  EXPECT_STREQ("ASF", TranscoderSettingsPage::TabLabel(Transcoder::Format::ASF));
  EXPECT_STREQ("flac", TranscoderSettingsPage::TabId(Transcoder::Format::FLAC));
  EXPECT_STREQ("mp3", TranscoderSettingsPage::TabId(Transcoder::Format::MP3));
  EXPECT_EQ(TranscoderSettingsPage::FieldKind::Quality, TranscoderSettingsPage::FieldsFor(Transcoder::Format::FLAC));
  EXPECT_EQ(TranscoderSettingsPage::FieldKind::Quality, TranscoderSettingsPage::FieldsFor(Transcoder::Format::OggVorbis));
  EXPECT_EQ(TranscoderSettingsPage::FieldKind::Bitrate, TranscoderSettingsPage::FieldsFor(Transcoder::Format::Opus));
  EXPECT_EQ(TranscoderSettingsPage::FieldKind::Bitrate, TranscoderSettingsPage::FieldsFor(Transcoder::Format::AAC));
  EXPECT_EQ(TranscoderSettingsPage::FieldKind::Mp3, TranscoderSettingsPage::FieldsFor(Transcoder::Format::MP3));
  EXPECT_EQ(8, TranscoderSettingsPage::QualityMax(Transcoder::Format::FLAC));
  EXPECT_EQ(10, TranscoderSettingsPage::QualityMax(Transcoder::Format::OggVorbis));
  EXPECT_STREQ("Compression (0–8)", TranscoderSettingsPage::QualityTitle(Transcoder::Format::FLAC));
  EXPECT_TRUE(std::string(TranscoderSettingsPage::IntroText()).find("Transcode Music") != std::string::npos);
  for (Transcoder::Format format : tabs) {
    EXPECT_TRUE(TranscoderSettingsPage::GroupMatches(format));
  }
  EXPECT_STREQ("audio/x-vorbis", TranscoderSettingsPage::FormatKeyFor(Transcoder::Format::OggVorbis));
  EXPECT_EQ(8u, TranscoderSettingsPage::DefaultFormatChoices().size());
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
  EXPECT_STREQ("MD5 token (Recommended)", StreamingChoices::SubsonicAuthMethods().back().second.c_str());
  EXPECT_EQ("highest", StreamingChoices::SomaFmQualities().front().first);
  EXPECT_EQ("aac-320", StreamingChoices::RadioParadiseStreams().front().first);
  ASSERT_EQ(5u, StreamingChoices::TidalCoverSizes().size());
  EXPECT_EQ("750x750", StreamingChoices::TidalCoverSizes()[3].first);
}

TEST(StreamingSettingsLabels, MatchQtStreamingAndRadioCopy) {
  EXPECT_STREQ("Enable", StreamingSettingsLabels::Enable());
  EXPECT_STREQ("Authentication", StreamingSettingsLabels::Authentication());
  EXPECT_STREQ("Preferences", StreamingSettingsLabels::Preferences());
  EXPECT_STREQ("Login", StreamingSettingsLabels::Login());
  EXPECT_STREQ("Search delay", StreamingSettingsLabels::SearchDelay());
  EXPECT_STREQ("Remove (Remastered), etc from song titles", StreamingSettingsLabels::RemoveRemastered());
  EXPECT_STREQ("Fetch entire albums when searching songs", StreamingSettingsLabels::FetchEntireAlbums());
  EXPECT_STREQ("App Secret", QobuzSettingsLabels::AppSecret());
  EXPECT_STREQ("Private key", QobuzSettingsLabels::PrivateKey());
  EXPECT_STREQ("Fetch Credentials", QobuzSettingsLabels::FetchCredentials());
  EXPECT_STREQ("Audio format", QobuzSettingsLabels::AudioFormat());
  EXPECT_STREQ("Missing app id. Please fetch credentials first.", QobuzSettingsLabels::MissingCredentialMessage("", "s", "k"));
  EXPECT_STREQ("Missing app secret. Please fetch credentials first.", QobuzSettingsLabels::MissingCredentialMessage("id", "", "k"));
  EXPECT_STREQ("Missing private key. Please fetch credentials first.", QobuzSettingsLabels::MissingCredentialMessage("id", "s", ""));
  EXPECT_EQ(nullptr, QobuzSettingsLabels::MissingCredentialMessage("id", "s", "k"));
  EXPECT_STREQ("Server URL", SubsonicSettingsLabels::ServerUrl());
  EXPECT_STREQ("Authentication method:", SubsonicSettingsLabels::AuthMethod());
  EXPECT_STREQ("Use HTTP/2 when possible", SubsonicSettingsLabels::Http2());
  EXPECT_STREQ("Verify server certificate", SubsonicSettingsLabels::VerifyCertificate());
  EXPECT_STREQ("Use album ID for album covers", SubsonicSettingsLabels::UseAlbumIdForCovers());
  EXPECT_EQ(SubsonicConnectionCheck::Result::MissingCredentials, SubsonicConnectionCheck::Validate("", "u", "p"));
  EXPECT_EQ(SubsonicConnectionCheck::Result::InvalidUrl, SubsonicConnectionCheck::Validate("not-a-url", "u", "p"));
  EXPECT_EQ(SubsonicConnectionCheck::Result::Ok, SubsonicConnectionCheck::Validate("https://music.example", "u", "p"));
  EXPECT_STREQ("Configuration incomplete", SubsonicConnectionCheck::Title(SubsonicConnectionCheck::Result::MissingCredentials));
  EXPECT_STREQ("Server URL is invalid.", SubsonicConnectionCheck::Body(SubsonicConnectionCheck::Result::InvalidUrl));
  EXPECT_STREQ("Delete songs", SubsonicSettingsActions::DeleteSongs());
  EXPECT_EQ(0, SubsonicSettingsActions::DeleteCachedSongs(nullptr));
  const std::string ping = SubsonicPing::Url("https://music.example/subsonic", "user", "secret", true);
  EXPECT_NE(std::string::npos, ping.find("/rest/ping.view"));
  EXPECT_NE(std::string::npos, ping.find("p=enc:"));
  const SubsonicPing::Result ok = SubsonicPing::Parse(R"json({"subsonic-response":{"status":"ok","version":"1.16.1"}})json");
  EXPECT_EQ(SubsonicPing::Status::Ok, ok.status);
  EXPECT_STREQ("Test successful!", SubsonicPing::Title(ok));
  EXPECT_EQ("Test successful!", SubsonicPing::Body(ok));
  const SubsonicPing::Result failed =
      SubsonicPing::Parse(R"json({"subsonic-response":{"status":"failed","error":{"code":40,"message":"Wrong username or password"}}})json");
  EXPECT_EQ(SubsonicPing::Status::Failed, failed.status);
  EXPECT_STREQ("Test failed!", SubsonicPing::Title(failed));
  EXPECT_EQ("Wrong username or password (40)", failed.message);
  const SubsonicPing::Result missing = SubsonicPing::Parse(R"json({"subsonic-response":{"version":"1.16.1"}})json");
  EXPECT_EQ(SubsonicPing::Status::Invalid, missing.status);
  EXPECT_EQ("Ping reply from server is missing status", missing.message);
  const SubsonicPing::Result http = SubsonicPing::Parse("", 503, "Service Unavailable");
  EXPECT_EQ(SubsonicPing::Status::Failed, http.status);
  EXPECT_EQ("Service Unavailable", http.message);
  EXPECT_STREQ("Tidal support is not official and requires a API token from a registered application to work. We can't help you getting these.",
              TidalSettingsLabels::Disclaimer());
  EXPECT_STREQ("Audio quality", TidalSettingsLabels::AudioQuality());
  EXPECT_STREQ("Album cover size", TidalSettingsLabels::AlbumCoverSize());
  EXPECT_STREQ("Basic authentication", SpotifySettingsLabels::BasicAuth());
  EXPECT_STREQ("Authenticate", SpotifySettingsLabels::Authenticate());
  EXPECT_STREQ("spotifyaudiosrc", SpotifySettingsLabels::PluginFeature());
  EXPECT_NE(std::string::npos, SpotifySettingsLabels::PluginWarningMarkup().find(SpotifySettingsLabels::PluginWikiUrl()));
  EXPECT_STREQ("Radios", RadioSettingsLabels::PageTitle());
  EXPECT_STREQ("Stream quality:", RadioSettingsLabels::StreamQuality());
  EXPECT_STREQ("Search results limit:", RadioSettingsLabels::SearchResultsLimit());
  EXPECT_STREQ("Default sort order:", RadioSettingsLabels::DefaultSortOrder());
  EXPECT_STREQ("Default country:", RadioSettingsLabels::DefaultCountry());
}

TEST(BackendOutputChoices, AppendsCustomAndDetectsUnlistedDevice) {
  EXPECT_STREQ("Custom", BackendOutputChoices::CustomLabel());
  EXPECT_STREQ("__custom__", BackendOutputChoices::CustomChoiceKey());
  EXPECT_TRUE(BackendOutputChoices::IsCustomKey(BackendOutputChoices::CustomChoiceKey()));
  EXPECT_FALSE(BackendOutputChoices::IsCustomKey("alsasink|hw:0,0"));

  const std::vector<AudioDevice> listed = {{"hw:0,0", "Card", "audio-card-symbolic", "alsasink"}};
  EXPECT_FALSE(BackendOutputChoices::DeviceIsCustom("alsasink", "", listed));
  EXPECT_FALSE(BackendOutputChoices::DeviceIsCustom("alsasink", "hw:0,0", listed));
  EXPECT_TRUE(BackendOutputChoices::DeviceIsCustom("alsasink", "hw:1,0", listed));
  EXPECT_EQ(BackendOutputChoices::CustomChoiceKey(), BackendOutputChoices::ComboKey("alsasink", "hw:1,0", listed));
  EXPECT_EQ("alsasink|hw:0,0", BackendOutputChoices::ComboKey("alsasink", "hw:0,0", listed));

  std::vector<std::pair<std::string, std::string>> devices = {{"alsasink|hw:0,0", "Card"}};
  BackendOutputChoices::AppendCustom(&devices);
  EXPECT_EQ(BackendOutputChoices::CustomChoiceKey(), devices.back().first);
  EXPECT_EQ("Custom", devices.back().second);
}

TEST(BehaviourStartupChoices, HideAndTrayInterlocksMatchQt) {
  const auto with_hide = BehaviourStartupChoices::StartupChoices(true, true);
  ASSERT_EQ(5u, with_hide.size());
  EXPECT_EQ("3", with_hide[2].first);
  EXPECT_EQ("Hide", with_hide[2].second);
  EXPECT_TRUE(BehaviourStartupChoices::IncludesHide(true, true));

  const auto no_hide = BehaviourStartupChoices::StartupChoices(true, false);
  ASSERT_EQ(4u, no_hide.size());
  EXPECT_EQ("2", no_hide[1].first);
  EXPECT_EQ("4", no_hide[2].first);
  EXPECT_FALSE(BehaviourStartupChoices::IncludesHide(false, true));
  EXPECT_FALSE(BehaviourStartupChoices::IncludesHide(true, false));

  EXPECT_EQ("1", BehaviourStartupChoices::EffectiveStartup("3", false, true));
  EXPECT_EQ("3", BehaviourStartupChoices::EffectiveStartup("3", true, true));
  EXPECT_TRUE(BehaviourStartupChoices::TrayDependentSensitive(true, true));
  EXPECT_FALSE(BehaviourStartupChoices::TrayDependentSensitive(true, false));
  EXPECT_FALSE(BehaviourStartupChoices::TrayDependentSensitive(false, true));
}

TEST(ContextFontControls, MatchesQtGroupsAndClampsPointSize) {
  EXPECT_STREQ("Font for headline", ContextFontControls::HeadlineGroup());
  EXPECT_STREQ("Font for data and lyrics", ContextFontControls::NormalGroup());
  EXPECT_STREQ("Font", ContextFontControls::FontTitle());
  EXPECT_STREQ("Font size", ContextFontControls::SizeTitle());
  EXPECT_STREQ(" pt", ContextFontControls::SizeSuffix());
  EXPECT_EQ(6.0, ContextFontControls::MinPt());
  EXPECT_EQ(32.0, ContextFontControls::MaxPt());
  EXPECT_EQ(0.5, ContextFontControls::Step());
  EXPECT_EQ(6.0, ContextFontControls::ClampPt(1.0));
  EXPECT_EQ(32.0, ContextFontControls::ClampPt(99.0));
  EXPECT_EQ(11.0, ContextFontControls::ClampPt(11.0));
  EXPECT_STREQ("Preview", ContextFontPreview::Title());
  EXPECT_STREQ("The quick brown fox jumps over the lazy dog", ContextFontPreview::HeadlineSample());
  EXPECT_STREQ("Lyrics and technical data use this font.", ContextFontPreview::NormalSample());
}

TEST(LoginStateVisibility, HidesCredentialsWhileSignedIn) {
  EXPECT_FALSE(LoginStateVisibility::ShowCredentials(LoginStateWidget::State::LoggedIn));
  EXPECT_TRUE(LoginStateVisibility::ShowCredentials(LoginStateWidget::State::LoggedOut));
  EXPECT_TRUE(LoginStateVisibility::ShowCredentials(LoginStateWidget::State::LoginInProgress));
  EXPECT_FALSE(LoginStateVisibility::CredentialsEnabled(LoginStateWidget::State::LoginInProgress));
  EXPECT_TRUE(LoginStateVisibility::CredentialsEnabled(LoginStateWidget::State::LoggedOut));
  EXPECT_STREQ("Signed in", LoginStateVisibility::StatusText(LoginStateWidget::State::LoggedIn));
  EXPECT_STREQ("Signing in…", LoginStateVisibility::StatusText(LoginStateWidget::State::LoginInProgress));
  EXPECT_STREQ("Not signed in", LoginStateVisibility::StatusText(LoginStateWidget::State::LoggedOut));
  EXPECT_TRUE(LoginStateVisibility::ShowLogin(LoginStateWidget::State::LoggedOut));
  EXPECT_TRUE(LoginStateVisibility::ShowLogout(LoginStateWidget::State::LoggedIn));
  EXPECT_TRUE(LoginStateVisibility::ShowProgress(LoginStateWidget::State::LoginInProgress));
}
