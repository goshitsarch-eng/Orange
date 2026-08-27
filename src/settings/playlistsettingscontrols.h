#ifndef STRAWBERRY_PLAYLISTSETTINGSCONTROLS_H
#define STRAWBERRY_PLAYLISTSETTINGSCONTROLS_H

namespace PlaylistSettingsControls {

inline bool GlowToggleEnabled(bool bars) { return bars; }
inline bool EffectiveGlow(bool bars, bool glow) { return bars && glow; }

}  // namespace PlaylistSettingsControls

#endif
