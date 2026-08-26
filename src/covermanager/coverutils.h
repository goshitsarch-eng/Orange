#ifndef STRAWBERRY_COVERUTILS_H
#define STRAWBERRY_COVERUTILS_H

#include <string>
#include <vector>

namespace CoverUtils {

std::string ExtensionForData(const std::vector<unsigned char> &data);
std::string ExtensionForData(const std::string &data);
bool LooksLikeImage(const std::string &data);

}  // namespace CoverUtils

#endif
