#include "core/commandlinefingerprint.h"
#include "core/commandlineoptions.h"
#include "core/commandlineshortopts.h"
#include "core/commandlineurl.h"
#include "core/commandlinevolume.h"
#include "core/commandlineurlplan.h"
#include "core/commandlinewindow.h"
#include "core/loadurl.h"
#include "core/playerplay.h"
#include "core/playlistsloadedgate.h"
#include "core/songloadurl.h"
#include "tidal/tidalloginurl.h"
#include "utilities/fileutils.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <cstring>
#include <vector>

TEST(CommandlineOptions, SetUrlsAreApplied) {
  CommandlineOptions options;
  EXPECT_TRUE(options.urls().empty());
  options.set_urls({"file:///tmp/a.flac", "https://example.invalid/stream"});
  ASSERT_EQ(2u, options.urls().size());
  EXPECT_EQ("file:///tmp/a.flac", options.urls().front());
  EXPECT_TRUE(options.contains_play_options());
}

TEST(CommandlineOptions, DefaultUrlListActionIsNone) {
  CommandlineOptions options;
  EXPECT_EQ(CommandlineOptions::UrlListAction::None, options.url_list_action());
}

TEST(CommandlineWindow, ParsesResizeAndRaise) {
  int width = 0;
  int height = 0;
  EXPECT_TRUE(CommandlineWindow::ParseSize("1280x720", &width, &height));
  EXPECT_EQ(1280, width);
  EXPECT_EQ(720, height);
  EXPECT_FALSE(CommandlineWindow::ParseSize("nope", &width, &height));
  EXPECT_TRUE(CommandlineWindow::ShouldResize(CommandlineOptions::PlayerAction::ResizeWindow, 800, 600));
  EXPECT_FALSE(CommandlineWindow::ShouldResize(CommandlineOptions::PlayerAction::Play, 800, 600));
  CommandlineOptions empty;
  EXPECT_TRUE(CommandlineWindow::ShouldRaise(empty));
  CommandlineOptions pause;
  pause.set_player_action(CommandlineOptions::PlayerAction::Pause);
  EXPECT_FALSE(CommandlineWindow::ShouldRaise(pause));
}

TEST(CommandlineOptions, ParsesResizeVersionAndLogLevels) {
  auto parse = [](std::vector<const char *> args) {
    args.insert(args.begin(), "strawberry");
    std::vector<char *> argv;
    for (const char *arg : args) {
      argv.push_back(const_cast<char *>(arg));
    }
    CommandlineOptions options;
    EXPECT_TRUE(options.Parse(static_cast<int>(argv.size()), argv.data()));
    return options;
  };
  const CommandlineOptions resize = parse({"--resize-window", "1024x768"});
  EXPECT_EQ(CommandlineOptions::PlayerAction::ResizeWindow, resize.player_action());
  EXPECT_EQ(1024, resize.resize_width());
  EXPECT_EQ(768, resize.resize_height());
  const CommandlineOptions version = parse({"--version"});
  EXPECT_TRUE(version.version());
  const CommandlineOptions verbose = parse({"--verbose"});
  EXPECT_EQ("*:4", verbose.log_levels());
  EXPECT_TRUE(verbose.debug());
  const CommandlineOptions quiet = parse({"--quiet"});
  EXPECT_EQ("*:1", quiet.log_levels());
  const CommandlineOptions load = parse({"--load", "file:///tmp/a.flac"});
  EXPECT_EQ(CommandlineOptions::UrlListAction::Load, load.url_list_action());
  EXPECT_EQ("file:///tmp/a.flac", load.urls().front());
  const CommandlineOptions append = parse({"--append", "file:///tmp/b.flac"});
  EXPECT_EQ(CommandlineOptions::UrlListAction::Append, append.url_list_action());
  const CommandlineOptions created = parse({"--create", "Mix", "file:///tmp/c.flac"});
  EXPECT_EQ(CommandlineOptions::UrlListAction::CreateNew, created.url_list_action());
  EXPECT_EQ("Mix", created.playlist_name());
}

TEST(CommandlineOptions, ShortPlayPauseStopAndOsdMatchQt) {
  auto parse = [](std::vector<const char *> args) {
    args.insert(args.begin(), "strawberry");
    std::vector<char *> argv;
    for (const char *arg : args) {
      argv.push_back(const_cast<char *>(arg));
    }
    CommandlineOptions options;
    EXPECT_TRUE(options.Parse(static_cast<int>(argv.size()), argv.data()));
    return options;
  };
  EXPECT_EQ(CommandlineOptions::PlayerAction::Play, parse({"-p"}).player_action());
  EXPECT_EQ(CommandlineOptions::PlayerAction::PlayPause, parse({"-t"}).player_action());
  EXPECT_EQ(CommandlineOptions::PlayerAction::Pause, parse({"-u"}).player_action());
  EXPECT_EQ(CommandlineOptions::PlayerAction::Stop, parse({"-s"}).player_action());
  EXPECT_EQ(CommandlineOptions::PlayerAction::Previous, parse({"-r"}).player_action());
  EXPECT_EQ(CommandlineOptions::PlayerAction::Next, parse({"-f"}).player_action());
  const CommandlineOptions track = parse({"-k", "2"});
  EXPECT_EQ(2, track.play_track_at());
  const CommandlineOptions playlist = parse({"-i", "Favorites"});
  EXPECT_EQ(CommandlineOptions::PlayerAction::PlayPlaylist, playlist.player_action());
  EXPECT_EQ("Favorites", playlist.playlist_name());
  EXPECT_TRUE(parse({"-o"}).show_osd());
  EXPECT_TRUE(parse({"-y"}).toggle_pretty_osd());
  EXPECT_EQ("nb", parse({"-g", "nb"}).language());
  const CommandlineOptions resize = parse({"-w", "800x600"});
  EXPECT_EQ(CommandlineOptions::PlayerAction::ResizeWindow, resize.player_action());
  EXPECT_EQ(800, resize.resize_width());
  EXPECT_EQ(600, resize.resize_height());
  EXPECT_TRUE(parse({"-v"}).version());
  EXPECT_EQ("*:1", parse({"-q"}).log_levels());
  EXPECT_EQ(CommandlineShortOpts::kPlay, CommandlineShortOpts::ForLongOption("play"));
  EXPECT_EQ(0, CommandlineShortOpts::ForLongOption("volume"));
  EXPECT_EQ(0, CommandlineShortOpts::ForLongOption("stop-after-current"));
  EXPECT_TRUE(CommandlineShortOpts::IsReserved('v'));
  EXPECT_TRUE(CommandlineShortOpts::IsReserved('q'));
  EXPECT_TRUE(CommandlineShortOpts::IsQtCompatible('p'));
  EXPECT_FALSE(CommandlineShortOpts::IsQtCompatible('v'));
}

TEST(CommandlineOptions, VolumeUpDownAndIncreaseByMatchQt) {
  auto parse = [](std::vector<const char *> args) {
    args.insert(args.begin(), "strawberry");
    std::vector<char *> argv;
    for (const char *arg : args) {
      argv.push_back(const_cast<char *>(arg));
    }
    CommandlineOptions options;
    EXPECT_TRUE(options.Parse(static_cast<int>(argv.size()), argv.data()));
    return options;
  };
  EXPECT_EQ(4, parse({"--volume-up"}).volume_modifier());
  EXPECT_EQ(-4, parse({"--volume-down"}).volume_modifier());
  EXPECT_EQ(10, parse({"--volume-increase-by", "10"}).volume_modifier());
  EXPECT_EQ(-7, parse({"--volume-decrease-by", "7"}).volume_modifier());
  EXPECT_EQ(5, parse({"--volume-increase", "5"}).volume_modifier());
  EXPECT_EQ(-3, parse({"--volume-decrease", "3"}).volume_modifier());
  EXPECT_EQ(4, CommandlineVolume::kUpDownStep);
  EXPECT_EQ(4, CommandlineVolume::Modifier(true, false, 10, 0));
  EXPECT_EQ(-4, CommandlineVolume::Modifier(false, true, 0, 10));
  EXPECT_EQ(10, CommandlineVolume::Modifier(false, false, 10, 0));
  EXPECT_EQ(-7, CommandlineVolume::Modifier(false, false, 0, 7));
  EXPECT_EQ(100, CommandlineVolume::Apply(98, 4));
  EXPECT_EQ(0, CommandlineVolume::Apply(2, -4));
  EXPECT_EQ(54, CommandlineVolume::Apply(50, 4));
}

TEST(CommandlineFingerprint, CreateFingerprintExitsWithoutStartingUi) {
  auto parse = [](std::vector<const char *> args) {
    args.insert(args.begin(), "strawberry");
    std::vector<char *> argv;
    for (const char *arg : args) {
      argv.push_back(const_cast<char *>(arg));
    }
    CommandlineOptions options;
    EXPECT_TRUE(options.Parse(static_cast<int>(argv.size()), argv.data()));
    return options;
  };
  const CommandlineOptions options = parse({"--create-fingerprint", "/tmp/track.flac"});
  EXPECT_EQ("/tmp/track.flac", options.create_fingerprint());
  EXPECT_TRUE(CommandlineFingerprint::ShouldRun(options.create_fingerprint()));
  EXPECT_FALSE(CommandlineFingerprint::ShouldRun({}));
  EXPECT_EQ(0, CommandlineFingerprint::ExitCode());
  EXPECT_EQ("ABC123\n", CommandlineFingerprint::StdoutLine("ABC123"));
  EXPECT_TRUE(CommandlineFingerprint::StdoutLine({}).empty());
}

TEST(CommandlineUrlPlan, MatchesQtLoadAppendCreateAndNone) {
  using Url = CommandlineOptions::UrlListAction;
  using Player = CommandlineOptions::PlayerAction;
  const auto none_never = CommandlineUrlPlan::FromOptions(Url::None, Player::None, BehaviourSettings::AddBehaviour::Append,
                                                          BehaviourSettings::PlayBehaviour::Never, false);
  EXPECT_FALSE(none_never.clear_current);
  EXPECT_EQ(CollectionBehaviour::Destination::Current, none_never.destination);
  EXPECT_FALSE(none_never.should_play);
  const auto play_urls = CommandlineUrlPlan::FromOptions(Url::None, Player::Play, BehaviourSettings::AddBehaviour::Append,
                                                         BehaviourSettings::PlayBehaviour::Never, false);
  EXPECT_TRUE(play_urls.should_play);
  EXPECT_TRUE(CommandlineUrlPlan::SkipStandalonePlay(true, Player::Play));
  EXPECT_FALSE(CommandlineUrlPlan::SkipStandalonePlay(false, Player::Play));
  const auto load = CommandlineUrlPlan::FromOptions(Url::Load, Player::None, BehaviourSettings::AddBehaviour::Append,
                                                    BehaviourSettings::PlayBehaviour::Never, false);
  EXPECT_TRUE(load.clear_current);
  EXPECT_FALSE(load.should_play);
  const auto created = CommandlineUrlPlan::FromOptions(Url::CreateNew, Player::Play, BehaviourSettings::AddBehaviour::Append,
                                                       BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, created.destination);
  EXPECT_TRUE(created.should_play);
  const auto enqueue = CommandlineUrlPlan::FromOptions(Url::None, Player::None, BehaviourSettings::AddBehaviour::Enqueue,
                                                       BehaviourSettings::PlayBehaviour::Never, false);
  EXPECT_EQ(CollectionBehaviour::QueueMode::Append, enqueue.queue);
  EXPECT_EQ("Mix", CommandlineUrlPlan::NewPlaylistName("Mix"));
  EXPECT_EQ("Playlist", CommandlineUrlPlan::NewPlaylistName(""));
  EXPECT_TRUE(CommandlineUrlPlan::PlayNow(Player::None, BehaviourSettings::PlayBehaviour::IfStopped, false));
  EXPECT_FALSE(CommandlineUrlPlan::PlayNow(Player::None, BehaviourSettings::PlayBehaviour::IfStopped, true));
}

TEST(PlaylistsLoadedGate, DefersCommandlineAndPlayUntilLoaded) {
  EXPECT_TRUE(PlaylistsLoadedGate::ShouldDeferCommandline(false));
  EXPECT_FALSE(PlaylistsLoadedGate::ShouldDeferCommandline(true));
  EXPECT_TRUE(PlaylistsLoadedGate::DeferPlay(false));
  EXPECT_FALSE(PlaylistsLoadedGate::DeferPlay(true));
  EXPECT_TRUE(PlaylistsLoadedGate::ShouldResumeAfterLoad(true, static_cast<int>(EngineBase::State::Playing)));
  EXPECT_TRUE(PlaylistsLoadedGate::ShouldResumeAfterLoad(true, static_cast<int>(EngineBase::State::Paused)));
  EXPECT_FALSE(PlaylistsLoadedGate::ShouldResumeAfterLoad(false, static_cast<int>(EngineBase::State::Playing)));
  EXPECT_FALSE(PlaylistsLoadedGate::ShouldResumeAfterLoad(true, static_cast<int>(EngineBase::State::Empty)));
  EXPECT_TRUE(PlaylistsLoadedGate::ShouldHonorPlayRequest(false, static_cast<int>(EngineBase::State::Playing), true));
  EXPECT_FALSE(PlaylistsLoadedGate::ShouldHonorPlayRequest(true, static_cast<int>(EngineBase::State::Playing), true));
  EXPECT_FALSE(PlaylistsLoadedGate::ShouldHonorPlayRequest(false, static_cast<int>(EngineBase::State::Empty), false));
}

TEST(PlayerPlay, DispatchesLikeQt) {
  EXPECT_EQ(PlayerPlay::Action::Seek, PlayerPlay::ForState(EngineBase::State::Playing));
  EXPECT_EQ(PlayerPlay::Action::UnPause, PlayerPlay::ForState(EngineBase::State::Paused));
  EXPECT_EQ(PlayerPlay::Action::Start, PlayerPlay::ForState(EngineBase::State::Empty));
  EXPECT_EQ(PlayerPlay::Action::Start, PlayerPlay::ForState(EngineBase::State::Idle));
  EXPECT_EQ(PlayerPlay::Action::Start, PlayerPlay::ForState(EngineBase::State::Error));
  EXPECT_EQ(0, PlayerPlay::SeekSeconds(0));
  EXPECT_EQ(5, PlayerPlay::SeekSeconds(5));
}

TEST(TidalLoginUrl, MatchesQtSchemeAndHost) {
  EXPECT_TRUE(TidalLoginUrl::IsLogin("tidal://login"));
  EXPECT_TRUE(TidalLoginUrl::IsLogin("tidal://login/auth"));
  EXPECT_TRUE(TidalLoginUrl::IsLogin("tidal://login/auth?code=abc&state=x"));
  EXPECT_TRUE(TidalLoginUrl::IsLogin("TIDAL://login/auth?code=abc"));
  EXPECT_FALSE(TidalLoginUrl::IsLogin("tidal://track/1"));
  EXPECT_FALSE(TidalLoginUrl::IsLogin("tidal://1"));
  EXPECT_FALSE(TidalLoginUrl::IsLogin("https://login.tidal.com/authorize"));
  EXPECT_FALSE(TidalLoginUrl::IsLogin("file:///tmp/a.flac"));
}

TEST(TidalLoginUrl, ConsumesCommandlineAndExtractsCode) {
  EXPECT_TRUE(TidalLoginUrl::ConsumesCommandline({"file:///a.flac", "tidal://login/auth?code=xyz"}));
  EXPECT_FALSE(TidalLoginUrl::ConsumesCommandline({"tidal://99", "file:///a.flac"}));
  EXPECT_EQ("tidal://login/auth?code=xyz", TidalLoginUrl::Find({"tidal://1", "tidal://login/auth?code=xyz"}));
  EXPECT_TRUE(TidalLoginUrl::Find({"tidal://99"}).empty());
  EXPECT_EQ("xyz", TidalLoginUrl::AuthorizationCode("tidal://login/auth?code=xyz&state=s"));
  EXPECT_EQ("a b", TidalLoginUrl::AuthorizationCode("tidal://login/auth?code=a%20b"));
  EXPECT_TRUE(TidalLoginUrl::AuthorizationCode("tidal://login/auth").empty());
  EXPECT_STREQ("tidal://login/auth", TidalLoginUrl::kRedirectUri);
  EXPECT_STREQ("https://login.tidal.com/oauth2/token", TidalLoginUrl::kAccessTokenUrl);
  const std::string request = TidalLoginUrl::AuthorizationRequestUrl("client-id", "challenge");
  EXPECT_NE(std::string::npos, request.find("https://login.tidal.com/authorize"));
  EXPECT_NE(std::string::npos, request.find("client_id=client-id"));
  EXPECT_NE(std::string::npos, request.find("redirect_uri="));
  EXPECT_NE(std::string::npos, request.find("tidal"));
  EXPECT_NE(std::string::npos, request.find("login"));
  EXPECT_NE(std::string::npos, request.find("code_challenge_method=S256"));
  EXPECT_NE(std::string::npos, request.find("code_challenge=challenge"));
  EXPECT_NE(std::string::npos, request.find("state=challenge"));
}

TEST(TidalLoginUrl, FailureForMatchesQtCallbackChecks) {
  EXPECT_EQ("access_denied", TidalLoginUrl::AuthorizationError("tidal://login/auth?error=access_denied"));
  EXPECT_EQ("User cancelled", TidalLoginUrl::AuthorizationError("tidal://login/auth?error=access_denied&error_description=User%20cancelled"));
  EXPECT_EQ("User cancelled", TidalLoginUrl::FailureFor("tidal://login/auth?error=access_denied&error_description=User%20cancelled", "s"));
  EXPECT_EQ("No authorization code", TidalLoginUrl::FailureFor("tidal://login/auth", "s"));
  EXPECT_EQ("Request URL is missing state!", TidalLoginUrl::FailureFor("tidal://login/auth?code=abc", "s"));
  EXPECT_EQ("Request URL has wrong state x != s", TidalLoginUrl::FailureFor("tidal://login/auth?code=abc&state=x", "s"));
  EXPECT_TRUE(TidalLoginUrl::FailureFor("tidal://login/auth?code=abc&state=s", "s").empty());
}

TEST(CommandlineUrl, ExistingFileBecomesAbsoluteFileUrl) {
  const std::string path = "/tmp/strawberry-cli-url-" + std::to_string(getpid()) + ".flac";
  ASSERT_TRUE(FileUtils::WriteFile(path, "x"));
  const std::string url = CommandlineUrl::FromArg(path);
  EXPECT_EQ(FileUtils::UriFromPath(FileUtils::CanonicalPath(path)), url);
  EXPECT_TRUE(CommandlineUrl::IsLocalFile(url));
  EXPECT_TRUE(CommandlineUrl::LocalFileExists(path));
  EXPECT_TRUE(CommandlineUrl::LocalFileExists(url));
  EXPECT_EQ("https://example.com/a.mp3", CommandlineUrl::FromArg("https://example.com/a.mp3"));
  EXPECT_EQ("http://not-a-file", CommandlineUrl::FromUserInput("not-a-file"));
  EXPECT_EQ("tidal://login/auth", CommandlineUrl::FromArg("tidal://login/auth"));
  unlink(path.c_str());
}

TEST(LoadUrl, ResolvesLocalTidalAndReject) {
  const std::string path = "/tmp/strawberry-loadurl-" + std::to_string(getpid()) + ".flac";
  ASSERT_TRUE(FileUtils::WriteFile(path, "x"));
  EXPECT_EQ(LoadUrl::Action::InsertLocal, LoadUrl::Resolve(path));
  EXPECT_TRUE(LoadUrl::ShouldInsert(path));
  EXPECT_EQ(LoadUrl::Action::TidalLogin, LoadUrl::Resolve("tidal://login/auth?code=abc"));
  EXPECT_TRUE(LoadUrl::ShouldInsert("tidal://login/auth"));
  EXPECT_EQ(LoadUrl::Action::Reject, LoadUrl::Resolve("file:///tmp/does-not-exist-strawberry-loadurl.flac"));
  EXPECT_FALSE(LoadUrl::ShouldInsert("/tmp/does-not-exist-strawberry-loadurl.flac"));
  EXPECT_STREQ("Can't open", LoadUrl::RejectMessage());
  EXPECT_EQ("File /missing.flac does not exist.", LoadUrl::MissingFileMessage("/missing.flac"));
  EXPECT_TRUE(SongLoadUrl::IsRawStreamScheme("rtsp"));
  EXPECT_TRUE(SongLoadUrl::ShouldAddAsRawStream("https://example.com/x"));
  EXPECT_FALSE(SongLoadUrl::ShouldAddAsRawStream("file:///tmp/a.flac"));
  unlink(path.c_str());
}

TEST(CommandlineOptions, ParseNormalizesExistingFileArg) {
  const std::string path = "/tmp/strawberry-cli-parse-" + std::to_string(getpid()) + ".flac";
  ASSERT_TRUE(FileUtils::WriteFile(path, "x"));
  char program[] = "strawberry";
  std::string arg = path;
  char *argv[] = {program, arg.data(), nullptr};
  CommandlineOptions options;
  ASSERT_TRUE(options.Parse(2, argv));
  ASSERT_EQ(1u, options.urls().size());
  EXPECT_EQ(CommandlineUrl::FromArg(path), options.urls().front());
  unlink(path.c_str());
}
