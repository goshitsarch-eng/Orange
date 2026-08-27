#ifndef STRAWBERRY_COMMANDLINEOPTIONS_H
#define STRAWBERRY_COMMANDLINEOPTIONS_H

#include <string>
#include <vector>

class CommandlineOptions {
 public:
  enum class UrlListAction { Append = 0, Load = 1, None = 2, CreateNew = 3 };
  enum class PlayerAction {
    None = 0,
    Play = 1,
    PlayPause = 2,
    Pause = 3,
    Stop = 4,
    Previous = 5,
    Next = 6,
    RestartOrPrevious = 7,
    StopAfterCurrent = 8,
    PlayPlaylist = 9,
    ResizeWindow = 10
  };

  CommandlineOptions() = default;
  bool Parse(int argc, char **argv);

  bool is_empty() const;
  bool contains_play_options() const;

  UrlListAction url_list_action() const { return url_list_action_; }
  PlayerAction player_action() const { return player_action_; }
  int set_volume() const { return set_volume_; }
  int volume_modifier() const { return volume_modifier_; }
  int seek_to() const { return seek_to_; }
  int seek_by() const { return seek_by_; }
  int play_track_at() const { return play_track_at_; }
  bool show_osd() const { return show_osd_; }
  bool toggle_pretty_osd() const { return toggle_pretty_osd_; }
  const std::vector<std::string> &urls() const { return urls_; }
  const std::string &language() const { return language_; }
  const std::string &log_levels() const { return log_levels_; }
  const std::string &playlist_name() const { return playlist_name_; }
  bool debug() const { return debug_; }
  bool version() const { return version_; }
  int resize_width() const { return resize_width_; }
  int resize_height() const { return resize_height_; }
  void set_urls(const std::vector<std::string> &urls) { urls_ = urls; }
  void set_player_action(PlayerAction action) { player_action_ = action; }
  void set_resize(int width, int height) {
    resize_width_ = width;
    resize_height_ = height;
    player_action_ = PlayerAction::ResizeWindow;
  }

 private:
  UrlListAction url_list_action_ = UrlListAction::Append;
  PlayerAction player_action_ = PlayerAction::None;
  int set_volume_ = -1;
  int volume_modifier_ = 0;
  int seek_to_ = -1;
  int seek_by_ = 0;
  int play_track_at_ = -1;
  bool show_osd_ = false;
  bool toggle_pretty_osd_ = false;
  bool debug_ = false;
  bool version_ = false;
  int resize_width_ = 0;
  int resize_height_ = 0;
  std::vector<std::string> urls_;
  std::string language_;
  std::string log_levels_;
  std::string playlist_name_;
};

#endif
