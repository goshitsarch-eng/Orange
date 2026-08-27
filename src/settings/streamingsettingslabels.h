#ifndef STRAWBERRY_STREAMINGSETTINGSLABELS_H
#define STRAWBERRY_STREAMINGSETTINGSLABELS_H

#include <string>

namespace StreamingSettingsLabels {

inline const char *Enable() { return "Enable"; }
inline const char *Authentication() { return "Authentication"; }
inline const char *Preferences() { return "Preferences"; }
inline const char *Login() { return "Login"; }
inline const char *SearchDelay() { return "Search delay"; }
inline const char *ArtistsSearchLimit() { return "Artists search limit"; }
inline const char *AlbumsSearchLimit() { return "Albums search limit"; }
inline const char *SongsSearchLimit() { return "Songs search limit"; }
inline const char *DownloadAlbumCovers() { return "Download album covers"; }
inline const char *RemoveRemastered() { return "Remove (Remastered), etc from song titles"; }
inline const char *FetchEntireAlbums() { return "Fetch entire albums when searching songs"; }
inline const char *AppId() { return "App ID"; }
inline const char *ClientId() { return "Client ID"; }
inline const char *Username() { return "Username"; }
inline const char *Password() { return "Password"; }

}  // namespace StreamingSettingsLabels

namespace QobuzSettingsLabels {

inline const char *AppSecret() { return "App Secret"; }
inline const char *PrivateKey() { return "Private key"; }
inline const char *FetchCredentials() { return "Fetch Credentials"; }
inline const char *FetchTooltip() { return "Automatically fetch app ID, app secret and private key from Qobuz web player"; }
inline const char *Fetching() { return "Fetching..."; }
inline const char *AudioFormat() { return "Audio format"; }
inline const char *ConfigIncomplete() { return "Configuration incomplete"; }
inline const char *MissingAppId() { return "Missing app id. Please fetch credentials first."; }
inline const char *MissingAppSecret() { return "Missing app secret. Please fetch credentials first."; }
inline const char *MissingPrivateKey() { return "Missing private key. Please fetch credentials first."; }
inline const char *CredentialsFetched() { return "Credentials fetched"; }
inline const char *CredentialsFetchedBody() {
  return "Credentials have been successfully fetched. Click Login to authenticate via your browser.";
}
inline const char *CredentialFetchFailed() { return "Credential fetch failed"; }

inline const char *MissingCredentialMessage(const std::string &app_id, const std::string &app_secret, const std::string &private_key) {
  if (app_id.empty()) {
    return MissingAppId();
  }
  if (app_secret.empty()) {
    return MissingAppSecret();
  }
  if (private_key.empty()) {
    return MissingPrivateKey();
  }
  return nullptr;
}

}  // namespace QobuzSettingsLabels

namespace SubsonicSettingsLabels {

inline const char *ServerUrl() { return "Server URL"; }
inline const char *AuthMethod() { return "Authentication method:"; }
inline const char *HexAuth() { return "Hex"; }
inline const char *Md5Auth() { return "MD5 token (Recommended)"; }
inline const char *Http2() { return "Use HTTP/2 when possible"; }
inline const char *VerifyCertificate() { return "Verify server certificate"; }
inline const char *UseAlbumIdForCovers() { return "Use album ID for album covers"; }
inline const char *ServerSideScrobbling() { return "Server-side scrobbling"; }
inline const char *Test() { return "Test"; }
inline const char *ConfigIncomplete() { return "Configuration incomplete"; }
inline const char *MissingCredentials() { return "Missing server url, username or password."; }
inline const char *ConfigIncorrect() { return "Configuration incorrect"; }
inline const char *InvalidUrl() { return "Server URL is invalid."; }
inline const char *TestSuccessful() { return "Test successful!"; }
inline const char *TestFailed() { return "Test failed!"; }

}  // namespace SubsonicSettingsLabels

namespace SubsonicConnectionCheck {

enum class Result { Ok, MissingCredentials, InvalidUrl };

inline bool HasUrlParts(const std::string &url) {
  return url.find("://") != std::string::npos && url.find("://") + 3 < url.size();
}

inline Result Validate(const std::string &url, const std::string &username, const std::string &password) {
  if (url.empty() || username.empty() || password.empty()) {
    return Result::MissingCredentials;
  }
  if (!HasUrlParts(url)) {
    return Result::InvalidUrl;
  }
  return Result::Ok;
}

inline const char *Title(Result result) {
  switch (result) {
    case Result::InvalidUrl:
      return SubsonicSettingsLabels::ConfigIncorrect();
    case Result::MissingCredentials:
      return SubsonicSettingsLabels::ConfigIncomplete();
    case Result::Ok:
      return SubsonicSettingsLabels::TestSuccessful();
  }
  return SubsonicSettingsLabels::ConfigIncomplete();
}

inline const char *Body(Result result) {
  switch (result) {
    case Result::InvalidUrl:
      return SubsonicSettingsLabels::InvalidUrl();
    case Result::MissingCredentials:
      return SubsonicSettingsLabels::MissingCredentials();
    case Result::Ok:
      return SubsonicSettingsLabels::TestSuccessful();
  }
  return SubsonicSettingsLabels::MissingCredentials();
}

}  // namespace SubsonicConnectionCheck

namespace TidalSettingsLabels {

inline const char *Disclaimer() {
  return "Tidal support is not official and requires a API token from a registered application to work. We can't help you getting these.";
}
inline const char *AudioQuality() { return "Audio quality"; }
inline const char *AlbumCoverSize() { return "Album cover size"; }
inline const char *StreamUrlMethod() { return "Stream URL method"; }
inline const char *AlbumExplicit() { return "Append explicit to album title for explicit albums"; }
inline const char *ConfigIncomplete() { return "Configuration incomplete"; }
inline const char *MissingClientId() { return "Missing Tidal client ID."; }

}  // namespace TidalSettingsLabels

namespace SpotifySettingsLabels {

inline const char *BasicAuth() { return "Basic authentication"; }
inline const char *Authenticate() { return "Authenticate"; }
inline const char *PluginFeature() { return "spotifyaudiosrc"; }
inline const char *PluginWikiUrl() { return "https://wiki.strawberrymusicplayer.org/wiki/Installing_GStreamer_Spotify_plugin"; }
inline const char *PluginWikiLabel() { return "Wiki"; }
inline const char *PluginWarning() {
  return "The GStreamer Spotify plugin is not detected, you will not be able to stream songs from Spotify without it.";
}
inline std::string PluginWarningMarkup() {
  return std::string(PluginWarning()) + " See <a href=\"" + PluginWikiUrl() + "\">" + PluginWikiLabel() +
         "</a> for instructions on how to install the plugin.";
}

}  // namespace SpotifySettingsLabels

namespace RadioSettingsLabels {

inline const char *PageTitle() { return "Radios"; }
inline const char *StreamQuality() { return "Stream quality:"; }
inline const char *SearchResultsLimit() { return "Search results limit:"; }
inline const char *HideBroken() { return "Hide broken stations"; }
inline const char *DefaultSortOrder() { return "Default sort order:"; }
inline const char *DefaultCountry() { return "Default country:"; }

}  // namespace RadioSettingsLabels

#endif  // STRAWBERRY_STREAMINGSETTINGSLABELS_H
