#ifndef STRAWBERRY_SMARTPLAYLISTACTIVATE_H
#define STRAWBERRY_SMARTPLAYLISTACTIVATE_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "widgets/listboxkeyboard.h"

// Qt SmartPlaylistsViewContainer connects doubleClicked only. Enter emits
// QAbstractItemView::activated with no handler. GTK row-activated is the
// double-click path; Enter must not run it. Add/play flags come from
// Behaviour → Double-click (same MimeData flags as Qt AddToPlaylist).

namespace SmartPlaylistActivate {

enum class Trigger { Enter, DoubleClick };

inline bool ShouldRun(Trigger trigger) { return trigger == Trigger::DoubleClick; }

inline bool IsEnter(unsigned keyval) {
  return keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter;
}

inline bool ActivateOnSingleClick() { return false; }

struct PlayParams {
  bool as_new = false;
  bool clear = false;
  bool should_play = false;
  bool enqueue = false;
  bool enqueue_next = false;
};

inline PlayParams FromPlan(const CollectionBehaviour::Plan &plan) {
  PlayParams params;
  params.as_new = plan.destination == CollectionBehaviour::Destination::New;
  params.clear = plan.clear_current;
  params.should_play = plan.should_play;
  params.enqueue = plan.queue == CollectionBehaviour::QueueMode::Append;
  params.enqueue_next = plan.queue == CollectionBehaviour::QueueMode::Next;
  return params;
}

inline PlayParams FromDoubleClick(BehaviourSettings::AddBehaviour add, BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  return FromPlan(CollectionBehaviour::FromDoubleClick(add, play, engine_stopped));
}

}  // namespace SmartPlaylistActivate

#endif  // STRAWBERRY_SMARTPLAYLISTACTIVATE_H
