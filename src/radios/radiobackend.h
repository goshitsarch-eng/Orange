#ifndef STRAWBERRY_RADIOBACKEND_H
#define STRAWBERRY_RADIOBACKEND_H

#include "core/database.h"
#include "radios/radiochannel.h"

#include <vector>

class RadioBackend {
 public:
  explicit RadioBackend(Database *database);

  std::vector<RadioChannel> Load() const;
  void Save(const RadioChannel &channel);
  void RemoveSource(Song::Source source);

 private:
  Database *database_ = nullptr;
};

#endif
