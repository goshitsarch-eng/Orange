#ifndef STRAWBERRY_NETWORKRESUME_H
#define STRAWBERRY_NETWORKRESUME_H

namespace NetworkResume {

// Qt QNetworkInformation::reachabilityChanged(Online) clears the QNAM
// connection and access caches so the next request does not reuse a stale
// keep-alive after suspend/resume.
inline const char *kNetworkChangedSignal = "network-changed";
inline const char *kUserAgent = "Strawberry/1.2.0 (+https://www.strawberrymusicplayer.org)";

inline bool ShouldClearConnectionCache(bool network_available) { return network_available; }

}  // namespace NetworkResume

#endif  // STRAWBERRY_NETWORKRESUME_H
