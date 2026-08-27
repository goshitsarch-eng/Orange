#ifndef STRAWBERRY_TAGWRITERFIELDS_H
#define STRAWBERRY_TAGWRITERFIELDS_H

namespace TagWriterFields {

inline const char *VorbisBpm() { return "BPM"; }
inline const char *VorbisMood() { return "MOOD"; }
inline const char *VorbisInitialKey() { return "INITIALKEY"; }
inline const char *VorbisOriginalYear() { return "ORIGINALYEAR"; }

inline const char *Id3Bpm() { return "TBPM"; }
inline const char *Id3InitialKey() { return "TKEY"; }
inline const char *Id3OriginalYear() { return "TORY"; }
inline const char *Id3Mood() { return "MOOD"; }

inline const char *ApeBpm() { return "BPM"; }
inline const char *ApeMood() { return "MOOD"; }
inline const char *ApeInitialKey() { return "INITIALKEY"; }
inline const char *ApeOriginalYear() { return "ORIGINALYEAR"; }

inline const char *Mp4Bpm() { return "tmpo"; }
inline const char *Mp4Mood() { return "----:com.apple.iTunes:MOOD"; }
inline const char *Mp4InitialKey() { return "----:com.apple.iTunes:initialkey"; }
inline const char *Mp4OriginalYear() { return "----:com.apple.iTunes:originalyear"; }

inline bool HasBpm(float bpm) { return bpm > 0.0f; }
inline bool HasOriginalYear(int year) { return year > 0; }

}  // namespace TagWriterFields

#endif  // STRAWBERRY_TAGWRITERFIELDS_H
