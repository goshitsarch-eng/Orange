#include "radios/radiobackend.h"

RadioBackend::RadioBackend(Database *database) : database_(database) {}

std::vector<RadioChannel> RadioBackend::Load() const {
  std::vector<RadioChannel> channels;
  if (!database_) {
    return channels;
  }
  SqlQuery query(database_, "SELECT source, name, url, thumbnail_url FROM radio_channels");
  while (query.Step()) {
    RadioChannel channel;
    channel.source = static_cast<Song::Source>(query.ColumnInt(0));
    channel.name = query.ColumnText(1);
    channel.url = query.ColumnText(2);
    channel.thumbnail_url = query.ColumnText(3);
    channels.push_back(channel);
  }
  return channels;
}

void RadioBackend::Save(const RadioChannel &channel) {
  if (!database_) {
    return;
  }
  SqlQuery query(database_, "INSERT INTO radio_channels (source, name, url, thumbnail_url) VALUES (?, ?, ?, ?)");
  query.Bind(1, static_cast<int>(channel.source));
  query.Bind(2, channel.name);
  query.Bind(3, channel.url);
  query.Bind(4, channel.thumbnail_url);
  query.Exec();
}

void RadioBackend::RemoveSource(Song::Source source) {
  if (!database_) {
    return;
  }
  SqlQuery query(database_, "DELETE FROM radio_channels WHERE source = ?");
  query.Bind(1, static_cast<int>(source));
  query.Exec();
}
