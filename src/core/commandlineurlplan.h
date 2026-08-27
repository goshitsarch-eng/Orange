#ifndef STRAWBERRY_COMMANDLINEURLPLAN_H
#define STRAWBERRY_COMMANDLINEURLPLAN_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "core/commandlineoptions.h"

#include <string>

namespace CommandlineUrlPlan {

// Qt CommandlineOptionsReceived: --play + URLs sets play_now and skips the standalone Play().
inline bool PlayNow(CommandlineOptions::PlayerAction player_action, BehaviourSettings::PlayBehaviour play_mode, bool playing) {
  if (player_action == CommandlineOptions::PlayerAction::Play) {
    return true;
  }
  switch (play_mode) {
    case BehaviourSettings::PlayBehaviour::Always:
      return true;
    case BehaviourSettings::PlayBehaviour::IfStopped:
      return !playing;
    case BehaviourSettings::PlayBehaviour::Never:
    default:
      return false;
  }
}

inline bool SkipStandalonePlay(bool has_urls, CommandlineOptions::PlayerAction action) {
  return has_urls && action == CommandlineOptions::PlayerAction::Play;
}

inline CollectionBehaviour::Plan FromOptions(CommandlineOptions::UrlListAction url_action, CommandlineOptions::PlayerAction player_action,
                                            BehaviourSettings::AddBehaviour add_mode, BehaviourSettings::PlayBehaviour play_mode, bool playing) {
  CollectionBehaviour::Plan plan;
  plan.should_play = PlayNow(player_action, play_mode, playing);
  switch (url_action) {
    case CommandlineOptions::UrlListAction::Load:
      plan.clear_current = true;
      break;
    case CommandlineOptions::UrlListAction::CreateNew:
      plan.destination = CollectionBehaviour::Destination::New;
      break;
    case CommandlineOptions::UrlListAction::None:
      plan = CollectionBehaviour::FromDoubleClick(add_mode, play_mode, !playing);
      if (player_action == CommandlineOptions::PlayerAction::Play) {
        plan.should_play = true;
      }
      break;
    case CommandlineOptions::UrlListAction::Append:
    default:
      break;
  }
  return plan;
}

// Qt MimeData::get_name_for_new_playlist: explicit name or "Playlist".
inline std::string NewPlaylistName(const std::string &playlist_name) { return playlist_name.empty() ? "Playlist" : playlist_name; }

}  // namespace CommandlineUrlPlan

#endif
