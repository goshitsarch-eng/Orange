#include "engine/enginebase.h"

void EngineBase::StartPreloading(const std::string &, const std::string &, bool, int64_t, int64_t) {}

void EngineBase::UpdateSpotifyAccessToken(const std::string &token) {
  spotify_access_token_ = token;
  SetSpotifyAccessToken();
}

void EngineBase::EmitAboutToFinish() {
  if (about_to_end_emitted_) {
    return;
  }
  about_to_end_emitted_ = true;
  TrackAboutToEnd.Emit();
}
