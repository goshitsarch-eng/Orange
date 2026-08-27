#ifndef STRAWBERRY_COMMANDLINEFINGERPRINT_H
#define STRAWBERRY_COMMANDLINEFINGERPRINT_H

#include <string>

namespace CommandlineFingerprint {

// Qt commandlineoptions.cpp CreateFingerPrint prints the fingerprint and exits 0.
inline bool ShouldRun(const std::string &filename) { return !filename.empty(); }

inline int ExitCode() { return 0; }

inline std::string StdoutLine(const std::string &fingerprint) {
  return fingerprint.empty() ? std::string() : fingerprint + "\n";
}

}  // namespace CommandlineFingerprint

#endif
