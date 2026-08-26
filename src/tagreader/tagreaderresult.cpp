#include "tagreader/tagreaderresult.h"

std::string TagReaderResult::error_string() const {
  switch (error_code) {
    case ErrorCode::Success:
      return "Success";
    case ErrorCode::Unsupported:
      return "File is unsupported";
    case ErrorCode::FilenameMissing:
      return "Filename is missing";
    case ErrorCode::FileDoesNotExist:
      return "File does not exist";
    case ErrorCode::FileOpenError:
      return "File could not be opened";
    case ErrorCode::FileParseError:
      return "Could not parse file";
    case ErrorCode::FileSaveError:
      return "Could not save file";
    case ErrorCode::CustomError:
      return error_text;
  }
  return "Unknown error";
}
