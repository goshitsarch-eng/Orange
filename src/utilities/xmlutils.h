#ifndef STRAWBERRY_XMLUTILS_H
#define STRAWBERRY_XMLUTILS_H

#include <string>

namespace XmlUtils {

std::string Escape(const std::string &value);
std::string Unescape(const std::string &value);
std::string Tag(const std::string &name, const std::string &value);

}  // namespace XmlUtils

#endif
