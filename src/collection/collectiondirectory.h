#ifndef STRAWBERRY_COLLECTIONDIRECTORY_H
#define STRAWBERRY_COLLECTIONDIRECTORY_H

#include <string>

struct CollectionDirectory {
  int id = -1;
  std::string path;
  bool subdirs = true;
};

#endif
