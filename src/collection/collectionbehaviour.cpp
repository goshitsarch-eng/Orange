#include "collection/collectionbehaviour.h"

namespace CollectionBehaviour {

bool ShouldPlay(BehaviourSettings::PlayBehaviour mode, bool engine_stopped) {
  switch (mode) {
    case BehaviourSettings::PlayBehaviour::Always:
      return true;
    case BehaviourSettings::PlayBehaviour::IfStopped:
      return engine_stopped;
    case BehaviourSettings::PlayBehaviour::Never:
    default:
      return false;
  }
}

Plan FromDoubleClick(BehaviourSettings::AddBehaviour add, BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  Plan plan;
  plan.should_play = ShouldPlay(play, engine_stopped);
  switch (add) {
    case BehaviourSettings::AddBehaviour::Enqueue:
      plan.queue = QueueMode::Append;
      break;
    case BehaviourSettings::AddBehaviour::Load:
      plan.clear_current = true;
      break;
    case BehaviourSettings::AddBehaviour::OpenInNew:
      plan.destination = Destination::New;
      break;
    case BehaviourSettings::AddBehaviour::Append:
    default:
      break;
  }
  return plan;
}

Plan Append(BehaviourSettings::PlayBehaviour menu_play, bool engine_stopped) {
  Plan plan;
  plan.should_play = ShouldPlay(menu_play, engine_stopped);
  return plan;
}

Plan Enqueue() {
  Plan plan;
  plan.queue = QueueMode::Append;
  return plan;
}

Plan EnqueueNext() {
  Plan plan;
  plan.queue = QueueMode::Next;
  return plan;
}

Plan OpenInNew(BehaviourSettings::PlayBehaviour menu_play, bool engine_stopped) {
  Plan plan;
  plan.destination = Destination::New;
  plan.should_play = ShouldPlay(menu_play, engine_stopped);
  return plan;
}

SongList UniqueByUrl(const SongList &songs) {
  SongList unique;
  for (const Song &song : songs) {
    bool found = false;
    for (const Song &existing : unique) {
      if (existing.url() == song.url()) {
        found = true;
        break;
      }
    }
    if (!found) {
      unique.push_back(song);
    }
  }
  return unique;
}

std::string NewPlaylistName(const SongList &songs) {
  if (songs.empty()) {
    return "Playlist";
  }
  if (!songs.front().album().empty()) {
    return songs.front().album();
  }
  if (!songs.front().EffectiveAlbumartist().empty()) {
    return songs.front().EffectiveAlbumartist();
  }
  return "Playlist";
}

namespace {

std::string Quoted(const std::string &value) { return "\"" + value + "\""; }

Song FirstSong(const CollectionItem *item) {
  if (!item) {
    return Song();
  }
  const SongList songs = item->Songs();
  return songs.empty() ? Song() : songs.front();
}

}  // namespace

std::string SearchQuery(const CollectionItem *item, const CollectionGrouping::Grouping &grouping) {
  if (!item) {
    return {};
  }
  if (item->type == CollectionItem::Type::Song) {
    return item->metadata.title().empty() ? item->display_text : "title:" + Quoted(item->metadata.title());
  }
  if (item->type != CollectionItem::Type::Container) {
    return item->display_text;
  }
  const Song song = FirstSong(item);
  const CollectionGrouping::GroupBy group_by = grouping[item->container_level];
  switch (group_by) {
    case CollectionGrouping::GroupBy::AlbumArtist:
      return "albumartist:" + Quoted(song.EffectiveAlbumartist());
    case CollectionGrouping::GroupBy::Artist:
      return "artist:" + Quoted(song.artist());
    case CollectionGrouping::GroupBy::Album:
    case CollectionGrouping::GroupBy::AlbumDisc:
      return "album:" + Quoted(song.album());
    case CollectionGrouping::GroupBy::YearAlbum:
    case CollectionGrouping::GroupBy::YearAlbumDisc:
      return "year:" + std::to_string(song.year()) + " album:" + Quoted(song.album());
    case CollectionGrouping::GroupBy::OriginalYearAlbum:
    case CollectionGrouping::GroupBy::OriginalYearAlbumDisc:
      return "year:" + std::to_string(CollectionGrouping::EffectiveOriginalYear(song)) + " album:" + Quoted(song.album());
    case CollectionGrouping::GroupBy::Year:
      return "year:" + std::to_string(song.year());
    case CollectionGrouping::GroupBy::OriginalYear:
      return "year:" + std::to_string(CollectionGrouping::EffectiveOriginalYear(song));
    case CollectionGrouping::GroupBy::Genre:
      return "genre:" + Quoted(song.genre());
    case CollectionGrouping::GroupBy::Composer:
      return "composer:" + Quoted(song.composer());
    case CollectionGrouping::GroupBy::Performer:
      return "performer:" + Quoted(song.performer());
    case CollectionGrouping::GroupBy::Grouping:
      return "grouping:" + Quoted(song.grouping());
    case CollectionGrouping::GroupBy::Samplerate:
      return "samplerate:" + std::to_string(song.samplerate());
    case CollectionGrouping::GroupBy::Bitdepth:
      return "bitdepth:" + std::to_string(song.bitdepth());
    case CollectionGrouping::GroupBy::Bitrate:
      return "bitrate:" + std::to_string(song.bitrate());
    default:
      return item->display_text;
  }
}

}  // namespace CollectionBehaviour
