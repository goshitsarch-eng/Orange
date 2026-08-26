#ifndef STRAWBERRY_PLAYLISTCOLUMNLAYOUT_H
#define STRAWBERRY_PLAYLISTCOLUMNLAYOUT_H

#include "playlist/playlistdelegates.h"

#include <string>
#include <vector>

class PlaylistColumnLayout {
 public:
  static std::vector<PlaylistColumn> DefaultVisible();
  static std::vector<PlaylistColumn> Visible();
  static bool IsVisible(PlaylistColumn column);
  static void SetVisibleColumns(const std::vector<PlaylistColumn> &columns);
  static void ToggleVisible(PlaylistColumn column);
  static void Hide(PlaylistColumn column);
  static bool Move(PlaylistColumn column, int delta);

  static PlaylistColumnAlign DefaultAlignment(PlaylistColumn column);
  static PlaylistColumnAlign Alignment(PlaylistColumn column);
  static void SetAlignment(PlaylistColumn column, PlaylistColumnAlign align);
  static float XAlign(PlaylistColumn column);

  static bool StretchEnabled();
  static void SetStretchEnabled(bool enabled);
  static bool StretchColumn(PlaylistColumn column);

  static bool RatingLocked();
  static void SetRatingLocked(bool locked);

  static void Reset();
  static PlaylistColumn FromTitle(const std::string &title);
};

#endif
