#ifndef STRAWBERRY_SAVETAGSOPTIONS_H
#define STRAWBERRY_SAVETAGSOPTIONS_H

enum class SaveTagsOption {
  NoType = 0,
  Tags = 1,
  Playcount = 2,
  Rating = 4,
  Cover = 8
};

using SaveTagsOptions = int;

inline SaveTagsOptions operator|(SaveTagsOption a, SaveTagsOption b) {
  return static_cast<int>(a) | static_cast<int>(b);
}

inline SaveTagsOptions operator|(SaveTagsOptions a, SaveTagsOption b) {
  return a | static_cast<int>(b);
}

inline SaveTagsOptions operator|(SaveTagsOption a, SaveTagsOptions b) {
  return static_cast<int>(a) | b;
}

inline bool HasSaveOption(SaveTagsOptions options, SaveTagsOption option) {
  return (options & static_cast<int>(option)) != 0;
}

#endif
