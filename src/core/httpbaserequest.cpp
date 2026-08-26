#include "core/httpbaserequest.h"

#include "utilities/strutils.h"

HttpBaseRequest::HttpBaseRequest(NetworkAccessManager *network) : network_(network) {}

std::string HttpBaseRequest::EncodeParams(const ParamList &params) {
  std::string out;
  for (const auto &param : params) {
    if (!out.empty()) {
      out += "&";
    }
    out += StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
  }
  return out;
}
