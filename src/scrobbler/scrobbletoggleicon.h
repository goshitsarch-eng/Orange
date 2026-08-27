#ifndef STRAWBERRY_SCROBBLETOGGLEICON_H
#define STRAWBERRY_SCROBBLETOGGLEICON_H

namespace ScrobbleToggleIcon {

inline const char *Name(bool enabled) { return enabled ? "document-send-symbolic" : "mail-send-symbolic"; }

}  // namespace ScrobbleToggleIcon

#endif
