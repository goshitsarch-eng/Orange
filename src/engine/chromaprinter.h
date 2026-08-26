#ifndef STRAWBERRY_CHROMAPRINTER_H
#define STRAWBERRY_CHROMAPRINTER_H

#include <string>

class Chromaprinter {
 public:
  explicit Chromaprinter(const std::string &url_or_filename);

  std::string CreateFingerprint();
  std::string CreateFullFingerprint();
  const std::string &LastError() const { return last_error_; }

 private:
  std::string CreateFingerprintInternal(bool legacy);

  std::string url_;
  std::string last_error_;
};

#endif
