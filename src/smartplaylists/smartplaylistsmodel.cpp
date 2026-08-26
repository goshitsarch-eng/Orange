#include "smartplaylists/smartplaylistsmodel.h"

std::vector<SmartPlaylistsItem> SmartPlaylistsModel::BuiltinItems() {
  std::vector<SmartPlaylistsItem> items;
  items.push_back({SmartPlaylistsItem::Kind::Builtin, "All songs", "all", {}});
  SmartPlaylistSearch never;
  never.terms.push_back({SmartPlaylistField::Playcount, SmartPlaylistOp::Equals, "0"});
  items.push_back({SmartPlaylistsItem::Kind::Builtin, "Never played", "never", never});
  SmartPlaylistSearch rated;
  rated.sort_field = SmartPlaylistField::Rating;
  rated.sort_descending = true;
  rated.limit = 100;
  items.push_back({SmartPlaylistsItem::Kind::Builtin, "Highest rated", "rated", rated});
  SmartPlaylistSearch newest;
  newest.sort_field = SmartPlaylistField::Year;
  newest.sort_descending = true;
  newest.limit = 100;
  items.push_back({SmartPlaylistsItem::Kind::Builtin, "Newest", "newest", newest});
  SmartPlaylistSearch played;
  played.sort_field = SmartPlaylistField::Playcount;
  played.sort_descending = true;
  played.limit = 100;
  items.push_back({SmartPlaylistsItem::Kind::Builtin, "Most played", "played", played});
  return items;
}

void SmartPlaylistsModel::RestoreDefaults() {
  SmartPlaylistSearch::SaveAll({});
  Reload();
}

void SmartPlaylistsModel::Reload() {
  items_ = BuiltinItems();
  for (const auto &preset : SmartPlaylistSearch::LoadSaved()) {
    SmartPlaylistsItem item;
    item.kind = SmartPlaylistsItem::Kind::Saved;
    item.title = preset.first;
    item.key = "saved:" + preset.first;
    item.search = preset.second;
    items_.push_back(item);
  }
  items_.push_back({SmartPlaylistsItem::Kind::Wizard, "Custom wizard…", "wizard", {}});
}

const SmartPlaylistsItem *SmartPlaylistsModel::ItemByKey(const std::string &key) const {
  for (const auto &item : items_) {
    if (item.key == key) {
      return &item;
    }
  }
  return nullptr;
}
