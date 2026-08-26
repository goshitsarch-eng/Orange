#include "utilities/xmlparser.h"

std::string XmlParser::ChildText(const std::string &xml, const std::string &tag) {
  const std::string open = "<" + tag + ">";
  const std::string close = "</" + tag + ">";
  const auto start = xml.find(open);
  const auto end = xml.find(close);
  if (start == std::string::npos || end == std::string::npos || end < start) {
    return {};
  }
  return xml.substr(start + open.size(), end - start - open.size());
}
