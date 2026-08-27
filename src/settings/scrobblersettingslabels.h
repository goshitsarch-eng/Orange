#ifndef STRAWBERRY_SCROBBLERSETTINGSLABELS_H
#define STRAWBERRY_SCROBBLERSETTINGSLABELS_H

namespace ScrobblerSettingsLabels {

inline const char *Enable() { return "Enable"; }
inline const char *ScrobbleInfo() {
  return "Songs are scrobbled if they have valid metadata and are longer than 30 seconds, have been playing for at least half its duration or for 4 minutes (whichever occurs earlier).";
}
inline const char *Offline() { return "Offline mode (Only cache scrobbles)"; }
inline const char *OfflineInfo() {
  return "With this option enabled, scrobbles will be cached to disk but not sent to the server. This option can be enabled in cases where the server or the internet connection is unstable, the scrobbles will be sent when the option is disabled.";
}
inline const char *ScrobbleButton() { return "Show scrobble button"; }
inline const char *LoveButton() { return "Show love button"; }
inline const char *SubmitEvery() { return "Submit scrobbles every"; }
inline const char *SubmitSeconds() { return " seconds"; }
inline const char *SubmitInfo() {
  return "This is the delay between when a song is scrobbled and when scrobbles are submitted to the server. Setting the time to 0 seconds will submit scrobbles immediately.";
}
inline const char *AlbumArtist() { return "Prefer album artist when sending scrobbles"; }
inline const char *ShowErrors() { return "Show dialog for errors"; }
inline const char *StripRemastered() { return "Strip \"remastered\" and similar from album and title"; }
inline const char *SourcesTitle() { return "Enable scrobbling for the following sources:"; }
inline const char *UserToken() { return "User token:"; }
inline const char *ListenBrainzTokenHint() { return "Enter your user token from https://listenbrainz.org/profile/"; }
inline const char *ListenBrainzProfileUrl() { return "https://listenbrainz.org/profile/"; }

}  // namespace ScrobblerSettingsLabels

#endif
