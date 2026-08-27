#ifndef STRAWBERRY_MIMEUTILS_H
#define STRAWBERRY_MIMEUTILS_H

#include <string>

namespace MimeUtils {

std::string MimeTypeFromData(const std::string &data);
std::string MimeTypeFromPath(const std::string &path);

}  // namespace MimeUtils

#endif
