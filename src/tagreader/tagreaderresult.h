#ifndef STRAWBERRY_TAGREADERRESULT_H
#define STRAWBERRY_TAGREADERRESULT_H

#include <string>

class TagReaderResult {
 public:
  enum class ErrorCode {
    Success,
    Unsupported,
    FilenameMissing,
    FileDoesNotExist,
    FileOpenError,
    FileParseError,
    FileSaveError,
    CustomError,
  };

  TagReaderResult(ErrorCode error_code = ErrorCode::Unsupported, const std::string &error_text = {})
      : error_code(error_code), error_text(error_text) {}

  bool success() const { return error_code == ErrorCode::Success; }
  std::string error_string() const;

  ErrorCode error_code;
  std::string error_text;
};

#endif
