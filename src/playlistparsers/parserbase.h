#ifndef STRAWBERRY_PARSERBASE_H
#define STRAWBERRY_PARSERBASE_H

#include "core/song.h"

#include <string>
#include <vector>

class ParserBase {
 public:
  virtual ~ParserBase() = default;

  virtual std::string name() const = 0;
  virtual std::vector<std::string> file_extensions() const = 0;
  virtual std::string mime_type() const { return {}; }
  virtual bool load_supported() const { return true; }
  virtual bool save_supported() const { return false; }
  virtual bool TryMagic(const std::string &data) const = 0;
  virtual SongList Load(const std::string &data, const std::string &playlist_path = {}) const = 0;
  virtual bool Save(const std::string &path, const SongList &songs) const { (void)path; (void)songs; return false; }

  static Song LoadSong(const std::string &playlist_dir, const std::string &entry);
  static void SetCollectionBackend(class CollectionBackend *backend);
  static void SetPathTypeOverride(int type);
  static std::string URLOrFilename(const std::string &url, const std::string &playlist_dir = {});
  static std::string XmlEscape(const std::string &value);
  static std::string XmlUnescape(const std::string &value);
  static std::string TagText(const std::string &data, const std::string &tag, size_t from, size_t *next = nullptr);
  static std::string AttributeValue(const std::string &element, const std::string &name);
  static bool HasUrlScheme(const std::string &value);
};

#endif
