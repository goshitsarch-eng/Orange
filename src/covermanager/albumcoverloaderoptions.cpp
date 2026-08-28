#include "covermanager/albumcoverloaderoptions.h"

#include "constants/coverssettings.h"
#include "core/settings.h"
#include "covermanager/coverarttypes.h"
#include "utilities/strutils.h"

AlbumCoverLoaderOptions::Types AlbumCoverLoaderOptions::LoadTypes() {
  Settings settings;
  settings.BeginGroup(CoversSettings::kSettingsGroup);
  const std::string value = settings.Value(CoversSettings::kTypes, CoverArtTypes::DefaultLoaderSaved());
  Types types;
  for (const std::string &part : StrUtils::Split(value, ',')) {
    const std::string name = StrUtils::Trim(part);
    if (name == "art_unset" || name == "art_embedded" || name == "art_manual" || name == "art_automatic") {
      types.push_back(TypeFromName(name));
    }
  }
  if (types.empty()) {
    for (const std::string &id : CoverArtTypes::LoaderDefaultIds()) {
      types.push_back(TypeFromName(id));
    }
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
