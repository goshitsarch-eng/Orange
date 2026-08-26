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
#include "dialogs/aboutcredits.h"
#include "qobuz/qobuzcredentialparser.h"
#include "core/mainwindowsponsor.h"
#include "settings/appearancesettingslabels.h"
#include "settings/backendsettingslabels.h"
#include "settings/behavioursettingslabels.h"
#include "collection/savedgroupinglabels.h"
#include "smartplaylists/smartplaylistslabel.h"
#include "settings/collectionsettingslabels.h"
#include "settings/contextsettingslabels.h"
#include "dialogs/errordialoglabels.h"
#include "transcoder/transcoderoptionslabels.h"
#include "settings/notificationscontrols.h"
#include "settings/notificationssettingslabels.h"
#include "settings/playlistsettingscontrols.h"
#include "settings/playlistsettingslabels.h"
#include "settings/scrobblersettingslabels.h"
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

TEST(AppearanceSettingsLabels, QtCopy) {
  EXPECT_STREQ("System icons", AppearanceSettingsLabels::SystemIcons());
  EXPECT_STREQ("You need to restart Strawberry for this setting to take affect", AppearanceSettingsLabels::DarkRestart());
  EXPECT_STREQ("You might need to restart Strawberry for this setting to fully apply", AppearanceSettingsLabels::StyleRestart());
  EXPECT_STREQ("Use the system default color set", AppearanceSettingsLabels::SystemColorSet());
  EXPECT_STREQ("Use a custom color set", AppearanceSettingsLabels::CustomColorSet());
  EXPECT_STREQ("Dark colors", AppearanceSettingsLabels::DarkColors());
  EXPECT_STREQ("Reset to default", AppearanceSettingsLabels::ResetColors());
  EXPECT_STREQ("Use gradient background", AppearanceSettingsLabels::TabBarGradient());
  EXPECT_STREQ("System highlight color", AppearanceSettingsLabels::PlaylistSystem());
  EXPECT_STREQ("Default background image", AppearanceSettingsLabels::DefaultBackground());
  EXPECT_STREQ("Upper Left", AppearanceSettingsLabels::UpperLeft());
  EXPECT_STREQ("Stretch image to fill playlist", AppearanceSettingsLabels::Stretch());
  EXPECT_STREQ("Do not cut image", AppearanceSettingsLabels::DoNotCut());
  EXPECT_STREQ("Max cover size", AppearanceSettingsLabels::MaxCoverSize());
  EXPECT_STREQ("Blur amount", AppearanceSettingsLabels::BlurAmount());
  EXPECT_STREQ("Play control buttons", AppearanceSettingsLabels::PlayControls());
  EXPECT_STREQ("Files, playlists and queue buttons", AppearanceSettingsLabels::FilesPlaylistsQueue());
  EXPECT_STREQ("Tabbar small mode", AppearanceSettingsLabels::TabbarSmall());
}

TEST(BackendSettingsLabels, QtCopy) {
  EXPECT_STREQ("Exclusive mode (Experimental)", BackendSettingsLabels::Exclusive());
  EXPECT_STREQ("Enable volume control", BackendSettingsLabels::VolumeControl());
  EXPECT_STREQ("Exponential volume scaling", BackendSettingsLabels::Exponential());
  EXPECT_STREQ("Upmix / downmix to", BackendSettingsLabels::ForceChannels());
  EXPECT_STREQ("Improve headphone listening of stereo audio records (bs2b)", BackendSettingsLabels::BS2B());
  EXPECT_STREQ("Use playbin3 when available", BackendSettingsLabels::Playbin3());
  EXPECT_STREQ("You need to restart Strawberry for this setting to take affect", BackendSettingsLabels::RestartHint());
  EXPECT_STREQ("Enable HTTP/2 for streaming", BackendSettingsLabels::HTTP2());
  EXPECT_STREQ("Use strict SSL mode", BackendSettingsLabels::StrictSSL());
  EXPECT_STREQ("No audio normalization", BackendSettingsLabels::NoNormalization());
  EXPECT_STREQ("Replay Gain", BackendSettingsLabels::ReplayGain());
  EXPECT_STREQ("Radio (equal loudness for all tracks)", BackendSettingsLabels::RadioMode());
  EXPECT_STREQ("Album (ideal loudness for all tracks)", BackendSettingsLabels::AlbumMode());
  EXPECT_STREQ("Apply compression to prevent clipping", BackendSettingsLabels::PreventClipping());
  EXPECT_STREQ("EBU R 128 Loudness Normalization", BackendSettingsLabels::Ebu());
  EXPECT_STREQ("Fade out when stopping a track", BackendSettingsLabels::FadeStop());
  EXPECT_STREQ("Cross-fade when changing tracks manually", BackendSettingsLabels::FadeManual());
  EXPECT_STREQ("Except between tracks on the same album or in the same CUE sheet", BackendSettingsLabels::FadeSameAlbum());
  EXPECT_STREQ("Fade out on pause / fade in on resume", BackendSettingsLabels::FadePause());
}

TEST(ContextSettingsLabels, EnableItemsCopy) {
  EXPECT_STREQ("Enable Items", ContextSettingsLabels::EnableItems());
  EXPECT_STREQ("Album", ContextSettingsLabels::Album());
  EXPECT_STREQ("Technical Data", ContextSettingsLabels::TechnicalData());
  EXPECT_STREQ("Song Lyrics", ContextSettingsLabels::SongLyrics());
  EXPECT_STREQ("Automatically search for album cover", ContextSettingsLabels::SearchCover());
  EXPECT_STREQ("Automatically search for song lyrics", ContextSettingsLabels::SearchLyrics());
  EXPECT_STREQ("Font for headline", ContextSettingsLabels::HeadlineFont());
  EXPECT_STREQ("Font for data and lyrics", ContextSettingsLabels::NormalFont());
}

TEST(ErrorDialogLabels, QtTitle) { EXPECT_STREQ("Strawberry Error", ErrorDialogLabels::Title()); }

TEST(TranscoderOptionsLabels, QtCopy) {
  EXPECT_STREQ("Transcoding options", TranscoderOptionsLabels::Title());
  EXPECT_STREQ("Optimize for quality", TranscoderOptionsLabels::Quality());
  EXPECT_STREQ("Optimize for bitrate", TranscoderOptionsLabels::OptimizeBitrate());
  EXPECT_STREQ("Force mono encoding", TranscoderOptionsLabels::ForceMono());
  EXPECT_STREQ("Encoding engine quality", TranscoderOptionsLabels::EngineQuality());
  EXPECT_STREQ("Fast", TranscoderOptionsLabels::EngineName(0));
  EXPECT_STREQ("Standard", TranscoderOptionsLabels::EngineName(1));
  EXPECT_STREQ("High", TranscoderOptionsLabels::EngineName(2));
  EXPECT_STREQ("Use bitrate management engine", TranscoderOptionsLabels::Managed());
  EXPECT_STREQ("Voice activity detection", TranscoderOptionsLabels::Vad());
  EXPECT_STREQ("Low complexity profile (LC)", TranscoderOptionsLabels::Lc());
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
  EXPECT_STREQ("ASF (WMA)", TranscoderSettingsPage::TabLabel(Transcoder::Format::ASF));
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
  EXPECT_EQ("Hide the main window", with_hide[2].second);
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
  EXPECT_STREQ("You are signed in.", LoginStateVisibility::StatusText(LoginStateWidget::State::LoggedIn));
  EXPECT_STREQ("Signing in...", LoginStateVisibility::StatusText(LoginStateWidget::State::LoginInProgress));
  EXPECT_STREQ("You are not signed in.", LoginStateVisibility::StatusText(LoginStateWidget::State::LoggedOut));
  EXPECT_STREQ("Sign out", LoginStateVisibility::SignOut());
  EXPECT_EQ("You are signed in.", LoginStateVisibility::SignedInAs({}));
  EXPECT_EQ("You are signed in as Jonas.", LoginStateVisibility::SignedInAs("Jonas"));
  EXPECT_TRUE(LoginStateVisibility::ShowLogin(LoginStateWidget::State::LoggedOut));
  EXPECT_TRUE(LoginStateVisibility::ShowLogout(LoginStateWidget::State::LoggedIn));
  EXPECT_TRUE(LoginStateVisibility::ShowProgress(LoginStateWidget::State::LoginInProgress));
}

TEST(AboutCredits, MatchesQtSectionsAndContributorCounts) {
  EXPECT_STREQ("About Strawberry", AboutCredits::WindowTitle());
  EXPECT_STREQ("Jonas Kvinge", AboutCredits::AuthorName());
  EXPECT_STREQ("Author and maintainer", AboutCredits::AuthorSection());
  EXPECT_STREQ("Contributors", AboutCredits::ContributorsSection());
  EXPECT_STREQ("Clementine authors", AboutCredits::ClementineAuthorsSection());
  EXPECT_STREQ("Clementine contributors", AboutCredits::ClementineContributorsSection());
  EXPECT_STREQ("Thanks to", AboutCredits::ThanksSection());
  EXPECT_STREQ("Thanks to all the other Amarok and Clementine contributors.", AboutCredits::ThanksOthers());
  EXPECT_STREQ("Jonas Kvinge", AboutCredits::Developers()[0]);
  EXPECT_EQ(55u, AboutCredits::Count(AboutCredits::StrawberryContributors()));
  EXPECT_STREQ("Gavin D. Howard", AboutCredits::StrawberryContributors()[0]);
  EXPECT_STREQ("Alex Bikadorov", AboutCredits::StrawberryContributors()[54]);
  EXPECT_EQ(4u, AboutCredits::Count(AboutCredits::ClementineAuthors()));
  EXPECT_STREQ("David Sansome", AboutCredits::ClementineAuthors()[0]);
  EXPECT_STREQ("Arnaud Bienner", AboutCredits::ClementineAuthors()[3]);
  EXPECT_EQ(19u, AboutCredits::Count(AboutCredits::ClementineContributors()));
  EXPECT_EQ(6u, AboutCredits::Count(AboutCredits::ThanksTo()));
  EXPECT_STREQ("https://github.com/strawberrymusicplayer/strawberry", AboutCredits::SourceUrl());
  EXPECT_NE(std::string::npos, std::string(AboutCredits::Comments()).find(AboutCredits::ForkNote()));
  EXPECT_NE(std::string::npos, std::string(AboutCredits::Comments()).find(AboutCredits::SponsorNote()));
}

TEST(QobuzCredentialParser, ExtractsBundlePathIdsSecretAndPrivateKey) {
  EXPECT_STREQ("https://play.qobuz.com/login", QobuzCredentialParser::LoginPageUrl());
  EXPECT_EQ("https://play.qobuz.com/resources/7.13.0-b123/bundle.js",
            QobuzCredentialParser::BundleUrl("/resources/7.13.0-b123/bundle.js"));
  EXPECT_EQ("/resources/7.13.0-b123/bundle.js",
            QobuzCredentialParser::ExtractBundlePath(R"(<script src="/resources/7.13.0-b123/bundle.js"></script>)"));
  EXPECT_TRUE(QobuzCredentialParser::ExtractBundlePath("<html></html>").empty());
  EXPECT_STREQ("Failed to find bundle.js URL in login page", QobuzCredentialParser::MissingBundle());
  EXPECT_EQ("123456789", QobuzCredentialParser::ExtractAppId(R"(production:{api:{appId:"123456789")"));
  EXPECT_EQ("98765432", QobuzCredentialParser::ExtractLoginAppId(R"({appId:"98765432"})"));
  EXPECT_EQ("6lz8C03UDIC7", QobuzCredentialParser::ExtractPrivateKey(R"(privateKey:"6lz8C03UDIC7")"));
  const std::string bundle =
      R"(a.initialSeed("MDEyMzQ1Njc4OWFiY2RlZjAxMjM0NTY3ODlhYmNkZWY=",window.utimezone.berlin))"
      R"(name:"Europe/Berlin",info:"X",extras:"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY")";
  EXPECT_EQ("0123456789abcdef0123456789abcdef", QobuzCredentialParser::ExtractAppSecret(bundle));
  EXPECT_TRUE(QobuzCredentialParser::ExtractAppSecret("no seeds").empty());
}

TEST(MainWindowSponsor, MatchesQtFirstRunCopy) {
  EXPECT_TRUE(MainWindowSponsor::ShouldShow(false));
  EXPECT_FALSE(MainWindowSponsor::ShouldShow(true));
  EXPECT_STREQ("Sponsoring Strawberry", MainWindowSponsor::Title());
  EXPECT_STREQ("Do not show this message again.", MainWindowSponsor::DoNotShowAgain());
  EXPECT_STREQ("https://www.strawberrymusicplayer.org/", MainWindowSponsor::WebsiteUrl());
  EXPECT_NE(std::string::npos, MainWindowSponsor::Message().find("please consider sponsoring the project"));
  EXPECT_STREQ("MainWindow", MainWindowSettings::kSettingsGroup);
  EXPECT_STREQ("do_not_show_sponsor_message", MainWindowSettings::kDoNotShowSponsorMessage);
}

TEST(NotificationsControls, ConvertsTimeoutAndGatesDuration) {
  EXPECT_STREQ("Strawberry can show a message when the track changes.", NotificationsSettingsLabels::Intro());
  EXPECT_STREQ("Show a native desktop notification", NotificationsSettingsLabels::Native());
  EXPECT_STREQ("Show a pretty OSD", NotificationsSettingsLabels::Pretty());
  EXPECT_STREQ("Popup duration", NotificationsSettingsLabels::PopupDuration());
  EXPECT_STREQ(" seconds", NotificationsSettingsLabels::Seconds());
  EXPECT_EQ(5, NotificationsControls::SecondsFromMs(5000));
  EXPECT_EQ(5000, NotificationsControls::MsFromSeconds(5));
  EXPECT_EQ(1, NotificationsControls::SecondsFromMs(100));
  EXPECT_EQ(20, NotificationsControls::SecondsFromMs(60000));
  EXPECT_FALSE(NotificationsControls::DurationSpinSensitive(OSDSettings::Type::Disabled, false));
  EXPECT_FALSE(NotificationsControls::DurationSpinSensitive(OSDSettings::Type::Pretty, true));
  EXPECT_TRUE(NotificationsControls::DurationSpinSensitive(OSDSettings::Type::Native, true));
}

TEST(PlaylistSettingsControls, GlowRequiresBars) {
  EXPECT_STREQ("Use alternating row colors", PlaylistSettingsLabels::Alternating());
  EXPECT_STREQ("Show a glowing animation on the currently playing track", PlaylistSettingsLabels::Glow());
  EXPECT_STREQ("When saving a playlist, file paths should be", PlaylistSettingsLabels::PathsGroup());
  EXPECT_FALSE(PlaylistSettingsControls::GlowToggleEnabled(false));
  EXPECT_TRUE(PlaylistSettingsControls::GlowToggleEnabled(true));
  EXPECT_FALSE(PlaylistSettingsControls::EffectiveGlow(false, true));
  EXPECT_TRUE(PlaylistSettingsControls::EffectiveGlow(true, true));
}

TEST(CollectionSettingsLabels, MatchQtCollectionCopy) {
  EXPECT_STREQ("These folders will be scanned for music to make up your collection", CollectionSettingsLabels::Intro());
  EXPECT_STREQ("Automatic updating", CollectionSettingsLabels::AutomaticUpdating());
  EXPECT_STREQ("Update the collection when Strawberry starts", CollectionSettingsLabels::StartupScan());
  EXPECT_STREQ("Display options", CollectionSettingsLabels::DisplayOptions());
  EXPECT_STREQ("Show album cover art in collection", CollectionSettingsLabels::PrettyCovers());
  EXPECT_STREQ("Album cover pixmap cache", CollectionSettingsLabels::CacheGroup());
  EXPECT_STREQ("Add new folder...", CollectionSettingsLabels::AddFolder());
  EXPECT_STREQ("Remove folder", CollectionSettingsLabels::RemoveFolder());
  EXPECT_STREQ("Enable delete files in the right click context menu", CollectionSettingsLabels::DeleteFiles());
  EXPECT_STREQ("Size", CollectionSettingsLabels::CacheSize());
  EXPECT_STREQ("Enable Disk Cache", CollectionSettingsLabels::EnableDiskCache());
  EXPECT_STREQ("Disk Cache Size", CollectionSettingsLabels::DiskCacheSize());
}

TEST(ScrobblerSettingsLabels, MatchQtSubmitDelayCopy) {
  EXPECT_STREQ("Submit scrobbles every", ScrobblerSettingsLabels::SubmitEvery());
  EXPECT_STREQ(" seconds", ScrobblerSettingsLabels::SubmitSeconds());
  EXPECT_STREQ("Offline mode (Only cache scrobbles)", ScrobblerSettingsLabels::Offline());
  EXPECT_STREQ("User token:", ScrobblerSettingsLabels::UserToken());
  EXPECT_STREQ("https://listenbrainz.org/profile/", ScrobblerSettingsLabels::ListenBrainzProfileUrl());
  EXPECT_NE(std::string::npos, std::string(ScrobblerSettingsLabels::ListenBrainzTokenHint()).find("listenbrainz.org/profile"));
}

TEST(BehaviourSettingsLabels, MatchQtPlaybackAndStartupCopy) {
  EXPECT_STREQ("Behavior", BehaviourSettingsLabels::PageTitle());
  EXPECT_STREQ("Keep running in the background when the window is closed", BehaviourSettingsLabels::KeepRunning());
  EXPECT_STREQ("Show song progress on system tray icon", BehaviourSettingsLabels::TrayProgress());
  EXPECT_STREQ("Show song progress on taskbar", BehaviourSettingsLabels::TaskbarProgress());
  EXPECT_STREQ("Resume playback on start", BehaviourSettingsLabels::ResumePlayback());
  EXPECT_STREQ("On startup", BehaviourSettingsLabels::OnStartup());
  EXPECT_STREQ("Remember from last time", BehaviourSettingsLabels::Remember());
  EXPECT_STREQ("Hide the main window", BehaviourSettingsLabels::HideWindow());
  EXPECT_STREQ("Use the system default", BehaviourSettingsLabels::SystemLanguage());
  EXPECT_STREQ("Using the menu to add a song will...", BehaviourSettingsLabels::MenuPlay());
  EXPECT_STREQ("Never start playing", BehaviourSettingsLabels::NeverPlay());
  EXPECT_STREQ("Play if there is nothing already playing", BehaviourSettingsLabels::PlayIfStopped());
  EXPECT_STREQ("Jump to previous song right away", BehaviourSettingsLabels::PreviousJump());
  EXPECT_STREQ("Restart song, then jump to previous if pressed again", BehaviourSettingsLabels::PreviousRestart());
  EXPECT_STREQ("Append to the playlist", BehaviourSettingsLabels::Append());
  EXPECT_STREQ("Replace the playlist", BehaviourSettingsLabels::Replace());
  EXPECT_STREQ("Add to the queue", BehaviourSettingsLabels::Enqueue());
  EXPECT_STREQ("Change the currently playing song", BehaviourSettingsLabels::ChangePlaying());
  EXPECT_STREQ("Seeking using a keyboard shortcut or mouse wheel", BehaviourSettingsLabels::Seeking());
  EXPECT_STREQ("Time step", BehaviourSettingsLabels::TimeStep());
  EXPECT_STREQ("Volume Increment", BehaviourSettingsLabels::VolumeIncrement());
  const auto add = BehaviourSettingsLabels::DoubleClickAddChoices();
  ASSERT_EQ(4u, add.size());
  EXPECT_EQ("1", add[0].first);
  EXPECT_EQ("3", add[1].first);
  EXPECT_EQ("4", add[2].first);
  EXPECT_EQ("2", add[3].first);
}

TEST(SavedGroupingLabels, MatchQtManagerCopy) {
  EXPECT_STREQ("Saved Grouping Manager", SavedGroupingLabels::Title());
  EXPECT_STREQ("Name", SavedGroupingLabels::Name());
  EXPECT_STREQ("First level", SavedGroupingLabels::FirstLevel());
  EXPECT_STREQ("Second Level", SavedGroupingLabels::SecondLevel());
  EXPECT_STREQ("Third Level", SavedGroupingLabels::ThirdLevel());
  EXPECT_STREQ("Remove", SavedGroupingLabels::Remove());
}

TEST(SmartPlaylistsLabel, MatchQtToolbarCopy) {
  EXPECT_STREQ("New smart playlist", SmartPlaylistsLabel::NewPlaylist());
  EXPECT_STREQ("Edit smart playlist", SmartPlaylistsLabel::EditPlaylist());
  EXPECT_STREQ("Delete smart playlist", SmartPlaylistsLabel::DeletePlaylist());
  EXPECT_STREQ("Restore defaults", SmartPlaylistsLabel::RestoreDefaults());
}
