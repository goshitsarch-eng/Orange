#ifndef STRAWBERRY_COVERMANAGERACTIONS_H
#define STRAWBERRY_COVERMANAGERACTIONS_H

namespace CoverManagerActions {

inline const char *WindowTitle() { return "Cover Manager"; }

inline bool DoubleClickShowsCover() { return true; }

inline bool ShowStatisticsWhenFetchFinishes(bool started, bool cancelled, size_t total) {
  return started && !cancelled && total > 0;
}

// Qt AlbumCoverManager::closeEvent: confirm before dismissing an in-flight fetch.
inline const char *CloseConfirmTitle() { return "Really cancel?"; }
inline const char *CloseConfirmMessage() { return "Closing this window will stop searching for album covers."; }
inline const char *CloseAbort() { return "Abort"; }
inline const char *CloseDontStop() { return "Don't stop!"; }

inline bool ShouldConfirmCloseOnFetch(bool running) { return running; }

// Qt DisableCoversButtons / EnableCoversButtons: fetch stays off while a fetch is running or no providers are registered.
inline bool FetchEnabled(bool running, bool has_providers) { return !running && has_providers; }
inline bool ExportEnabled(bool running) { return !running; }
inline bool CanCloseWithoutConfirm(bool running) { return !running; }

}  // namespace CoverManagerActions

#endif  // STRAWBERRY_COVERMANAGERACTIONS_H
