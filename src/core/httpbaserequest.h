#ifndef STRAWBERRY_HTTPBASEREQUEST_H
#define STRAWBERRY_HTTPBASEREQUEST_H

#include "core/network.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class HttpBaseRequest {
 public:
  using Param = std::pair<std::string, std::string>;
  using ParamList = std::vector<Param>;

  enum class ErrorCode { Success, NetworkError, HttpError, APIError, ParseError };

  struct HttpBaseRequestResult {
    ErrorCode error_code = ErrorCode::Success;
    int http_status_code = 200;
    std::string error_message;
    bool success() const { return error_code == ErrorCode::Success; }
  };

  struct ReplyDataResult : HttpBaseRequestResult {
    std::string data;
  };

  explicit HttpBaseRequest(NetworkAccessManager *network);
  virtual ~HttpBaseRequest() = default;
  virtual std::string service_name() const = 0;
  virtual bool authentication_required() const { return false; }
  static std::string EncodeParams(const ParamList &params);

 protected:
  NetworkAccessManager *network_ = nullptr;
};

#endif
