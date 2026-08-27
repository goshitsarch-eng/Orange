#ifndef STRAWBERRY_DATABASESCHEMA_H
#define STRAWBERRY_DATABASESCHEMA_H

#include <string>
#include <vector>

namespace DatabaseSchema {

constexpr int kMinSupportedSchemaVersion = 10;
constexpr char kMagicAllSongsTables[] = "%allsongstables";
constexpr char kResourcePrefix[] = "/org/strawberrymusicplayer/Strawberry/schema/";

inline std::string ResourcePath(int version) {
  if (version <= 0) {
    return std::string(kResourcePrefix) + "schema.sql";
  }
  return std::string(kResourcePrefix) + "schema-" + std::to_string(version) + ".sql";
}

inline bool IsEmptyDatabase(int user_table_count) { return user_table_count == 0; }

inline bool IsSupported(int version) { return version >= kMinSupportedSchemaVersion; }

inline std::vector<int> IncrementalVersions(int from, int to) {
  std::vector<int> versions;
  for (int version = from + 1; version <= to; ++version) {
    versions.push_back(version);
  }
  return versions;
}

inline std::string NormalizeNewlines(std::string schema) {
  std::string::size_type pos = 0;
  while ((pos = schema.find("\r\n", pos)) != std::string::npos) {
    schema.replace(pos, 2, "\n");
  }
  return schema;
}

inline std::string Trim(const std::string &text) {
  const std::string::size_type begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

inline bool IsCommandDelimiter(const std::string &schema, std::string::size_type semicolon) {
  std::string::size_type i = semicolon + 1;
  while (i < schema.size() && schema[i] == ' ') {
    ++i;
  }
  return i + 1 < schema.size() && schema[i] == '\n' && schema[i + 1] == '\n';
}

inline std::string::size_type DelimiterEnd(const std::string &schema, std::string::size_type semicolon) {
  std::string::size_type i = semicolon + 1;
  while (i < schema.size() && schema[i] == ' ') {
    ++i;
  }
  return i + 2;
}

// Qt Database::ExecSchemaCommands splits on the regex "; *\n\n".
inline std::vector<std::string> SplitCommands(const std::string &schema) {
  const std::string normalized = NormalizeNewlines(schema);
  std::vector<std::string> commands;
  std::string::size_type start = 0;
  while (start < normalized.size()) {
    std::string::size_type semicolon = start;
    bool found = false;
    while ((semicolon = normalized.find(';', semicolon)) != std::string::npos) {
      if (IsCommandDelimiter(normalized, semicolon)) {
        const std::string command = Trim(normalized.substr(start, semicolon - start));
        if (!command.empty()) {
          commands.push_back(command);
        }
        start = DelimiterEnd(normalized, semicolon);
        found = true;
        break;
      }
      ++semicolon;
    }
    if (!found) {
      const std::string command = Trim(normalized.substr(start));
      if (!command.empty()) {
        commands.push_back(command);
      }
      break;
    }
  }
  return commands;
}

inline bool ContainsMagic(const std::string &command) { return command.find(kMagicAllSongsTables) != std::string::npos; }

inline std::string ExpandMagic(const std::string &command, const std::string &table) {
  std::string expanded = command;
  const std::string magic = kMagicAllSongsTables;
  std::string::size_type pos = 0;
  while ((pos = expanded.find(magic, pos)) != std::string::npos) {
    expanded.replace(pos, magic.size(), table);
    pos += table.size();
  }
  return expanded;
}

inline bool IsSongsTable(const std::string &name) {
  return name == "songs" || (name.size() >= 6 && name.compare(name.size() - 6, 6, "_songs") == 0);
}

// Qt Database::SongsTables: songs, *_songs, then playlist_items.
inline std::vector<std::string> SongsTables(const std::vector<std::string> &tables) {
  std::vector<std::string> song_tables;
  for (const std::string &table : tables) {
    if (IsSongsTable(table)) {
      song_tables.push_back(table);
    }
  }
  song_tables.emplace_back("playlist_items");
  return song_tables;
}

inline std::vector<std::string> ExpandCommand(const std::string &command, const std::vector<std::string> &song_tables) {
  if (!ContainsMagic(command)) {
    return {command};
  }
  std::vector<std::string> expanded;
  expanded.reserve(song_tables.size());
  for (const std::string &table : song_tables) {
    expanded.push_back(ExpandMagic(command, table));
  }
  return expanded;
}

}  // namespace DatabaseSchema

#endif
