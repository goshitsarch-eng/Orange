#include "core/commandlineoptions.h"
#include "core/commandlineurlplan.h"
#include "core/commandlinewindow.h"
#include "core/playlistsloadedgate.h"
#include "tidal/tidalloginurl.h"

#include <gtest/gtest.h>

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
  const std::string request = TidalLoginUrl::AuthorizationRequestUrl("client-id");
  EXPECT_NE(std::string::npos, request.find("https://login.tidal.com/authorize"));
  EXPECT_NE(std::string::npos, request.find("client_id=client-id"));
  EXPECT_NE(std::string::npos, request.find("redirect_uri="));
  EXPECT_NE(std::string::npos, request.find("tidal"));
  EXPECT_NE(std::string::npos, request.find("login"));
}
