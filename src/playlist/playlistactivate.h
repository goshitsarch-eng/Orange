#ifndef STRAWBERRY_PLAYLISTACTIVATE_H
#define STRAWBERRY_PLAYLISTACTIVATE_H

#include "constants/behavioursettings.h"

namespace PlaylistActivate {

// Qt MainWindow::PlaylistDoubleClick
enum class Action { Play, Enqueue, EnqueueAndPlay };

inline Action Resolve(BehaviourSettings::PlaylistAddBehaviour mode, bool playing) {
  if (mode != BehaviourSettings::PlaylistAddBehaviour::Enqueue) {
    return Action::Play;
  }
  return playing ? Action::Enqueue : Action::EnqueueAndPlay;
}

}  // namespace PlaylistActivate

#endif
