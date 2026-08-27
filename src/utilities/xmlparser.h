#ifndef STRAWBERRY_XMLPARSER_H
#define STRAWBERRY_XMLPARSER_H

#include "utilities/xmlutils.h"

class XmlParser {
 public:
  static std::string ChildText(const std::string &xml, const std::string &tag);
};

#endif
