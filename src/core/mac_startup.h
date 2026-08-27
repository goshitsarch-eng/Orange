#ifndef STRAWBERRY_MAC_STARTUP_H
#define STRAWBERRY_MAC_STARTUP_H
#ifdef __APPLE__
class PlatformInterface;
class GlobalShortcutsBackendMacOs;

void MacStartup();
void MacSetApplicationHandler(PlatformInterface *handler);
void MacSetShortcutHandler(GlobalShortcutsBackendMacOs *handler);
void MacSetDockMenu(void *nsmenu);
void MacHandleMediaEvent(void *nsevent);
#endif
#endif
