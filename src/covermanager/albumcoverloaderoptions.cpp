#include "covermanager/albumcoverloaderoptions.h"

#include "core/settings.h"
#include "utilities/strutils.h"

AlbumCoverLoaderOptions::Types AlbumCoverLoaderOptions::LoadTypes() {
  Settings settings;
  settings.BeginGroup("Covers");
  const std::string value = settings.Value("types", "art_embedded,art_automatic,art_manual");
  Types types;
  for (const std::string &part : StrUtils::Split(value, ',')) {
    const std::string name = StrUtils::Trim(part);
    if (name == "art_unset" || name == "art_embedded" || name == "art_manual" || name == "art_automatic") {
      types.push_back(TypeFromName(name));
    }
  }
  if (types.empty()) {
    types = {Type::Embedded, Type::Automatic, Type::Manual};
  }
  return types;
}

std::string AlbumCoverLoaderOptions::TypeName(Type type) {
  switch (type) {
    case Type::Unset:
      return "art_unset";
    case Type::Embedded:
      return "art_embedded";
    case Type::Manual:
      return "art_manual";
    case Type::Automatic:
      return "art_automatic";
  }
  return {};
}

AlbumCoverLoaderOptions::Type AlbumCoverLoaderOptions::TypeFromName(const std::string &name) {
  if (name == "art_unset") {
    return Type::Unset;
  }
  if (name == "art_embedded") {
    return Type::Embedded;
  }
  if (name == "art_manual") {
    return Type::Manual;
  }
  return Type::Automatic;
}
