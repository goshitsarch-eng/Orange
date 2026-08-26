#ifndef STRAWBERRY_ALBUMCOVERLOADEROPTIONS_H
#define STRAWBERRY_ALBUMCOVERLOADEROPTIONS_H

#include <string>
#include <vector>

class AlbumCoverLoaderOptions {
 public:
  enum class Option {
    NoOptions = 0x0,
    RawImageData = 0x2,
    OriginalImage = 0x4,
    ScaledImage = 0x8,
    PadScaledImage = 0x10
  };

  enum class Type {
    Embedded,
    Automatic,
    Manual,
    Unset
  };

  using Types = std::vector<Type>;

  int options = static_cast<int>(Option::ScaledImage);
  int desired_scaled_size = 32;
  Types types = {Type::Embedded, Type::Automatic, Type::Manual};

  static Types LoadTypes();
  static std::string TypeName(Type type);
  static Type TypeFromName(const std::string &name);
};

#endif
