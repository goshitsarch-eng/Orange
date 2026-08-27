#ifndef STRAWBERRY_RADIOTREELEFT_H
#define STRAWBERRY_RADIOTREELEFT_H

#include "collection/collectiontreeleft.h"
#include "radios/radiotree.h"

namespace RadioTreeLeft {

using Action = CollectionTreeLeft::Action;

// Qt AutoExpandingTreeView Left: services are root containers, channels are leaves.
inline Action FromRow(RadioTree::Kind kind, bool service_expanded) {
  const bool service = kind == RadioTree::Kind::Service;
  return CollectionTreeLeft::FromState(service, service && service_expanded, service);
}

inline bool ShouldCollapse(RadioTree::Kind kind, bool service_expanded) {
  return FromRow(kind, service_expanded) != Action::None;
}

inline bool ShouldExpand(RadioTree::Kind kind, bool service_expanded) {
  return kind == RadioTree::Kind::Service && !service_expanded;
}

}  // namespace RadioTreeLeft

#endif
