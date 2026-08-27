#ifndef STRAWBERRY_MACSTARTUPACTIONS_H
#define STRAWBERRY_MACSTARTUPACTIONS_H

#include "systemtrayicon/systemtrayicon.h"

#include <vector>

namespace MacStartupActions {

// Qt AppDelegate::applicationShouldHandleReopen always Activates and returns YES.
inline bool ShouldHandleReopen() { return true; }

inline bool ReopenReturnYes() { return true; }

// Qt applicationShouldTerminate returns NSTerminateNow.
inline bool ShouldTerminateNow() { return true; }

// Qt handleMediaEvent: NSEventTypeSystemDefined subtype 8, released when ((flags & 0xFF00) >> 8) == 0xB.
inline int AuxControlSubtype() { return 8; }

inline bool IsAuxControlSubtype(int subtype) { return subtype == AuxControlSubtype(); }

inline int MediaKeyCode(int data1) { return (data1 & 0xFFFF0000) >> 16; }

inline int MediaKeyFlags(int data1) { return data1 & 0x0000FFFF; }

inline bool IsMediaKeyReleased(int flags) { return ((flags & 0xFF00) >> 8) == 0xB; }

inline bool ShouldHandleMediaEvent(int subtype, int flags) { return IsAuxControlSubtype(subtype) && IsMediaKeyReleased(flags); }

inline std::vector<int> DockMenuIds(bool show_love) { return SystemTrayIcon::RootMenuIds(show_love); }

inline bool DockItemIsSeparator(int id) { return SystemTrayIcon::IsSeparatorId(id); }

inline const char *NowPlayingLabel() { return "Now Playing"; }

}  // namespace MacStartupActions

#endif
