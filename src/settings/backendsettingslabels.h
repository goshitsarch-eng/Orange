#ifndef STRAWBERRY_BACKENDSETTINGSLABELS_H
#define STRAWBERRY_BACKENDSETTINGSLABELS_H

namespace BackendSettingsLabels {

inline const char *Exclusive() { return "Exclusive mode (Experimental)"; }
inline const char *VolumeControl() { return "Enable volume control"; }
inline const char *Exponential() { return "Exponential volume scaling"; }
inline const char *ExponentialHint() {
  return "Map the volume slider to a decibel scale so that perceived loudness changes evenly. Each 1% step equals 0.5 dB, 100% is 0 dB and 0% is silence.";
}
inline const char *ForceChannels() { return "Upmix / downmix to"; }
inline const char *Channels() { return "channels"; }
inline const char *BS2B() { return "Improve headphone listening of stereo audio records (bs2b)"; }
inline const char *Playbin3() { return "Use playbin3 when available"; }
inline const char *RestartHint() { return "You need to restart Orange for this setting to take affect"; }
inline const char *HTTP2() { return "Enable HTTP/2 for streaming"; }
inline const char *StrictSSL() { return "Use strict SSL mode"; }
inline const char *NoNormalization() { return "No audio normalization"; }
inline const char *ReplayGain() { return "Replay Gain"; }
inline const char *ReplayGainHint() { return "Use Replay Gain metadata if it is available"; }
inline const char *ReplayGainMode() { return "Replay Gain mode"; }
inline const char *RadioMode() { return "Radio (equal loudness for all tracks)"; }
inline const char *AlbumMode() { return "Album (ideal loudness for all tracks)"; }
inline const char *Preamp() { return "Pre-amp"; }
inline const char *PreventClipping() { return "Apply compression to prevent clipping"; }
inline const char *FallbackGain() { return "Fallback-gain"; }
inline const char *Ebu() { return "EBU R 128 Loudness Normalization"; }
inline const char *EbuHint() { return "Perform track loudness normalization"; }
inline const char *TargetLevel() { return "Target Level"; }
inline const char *FadeStop() { return "Fade out when stopping a track"; }
inline const char *FadeManual() { return "Cross-fade when changing tracks manually"; }
inline const char *FadeAuto() { return "Cross-fade when changing tracks automatically"; }
inline const char *FadeSameAlbum() { return "Except between tracks on the same album or in the same CUE sheet"; }
inline const char *FadeDuration() { return "Fading duration"; }
inline const char *FadePause() { return "Fade out on pause / fade in on resume"; }

}  // namespace BackendSettingsLabels

#endif
