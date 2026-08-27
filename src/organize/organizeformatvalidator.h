#ifndef STRAWBERRY_ORGANIZEFORMATVALIDATOR_H
#define STRAWBERRY_ORGANIZEFORMATVALIDATOR_H

#include <string>

class OrganizeFormatValidator {
 public:
  static bool IsValid(const std::string &format, std::string *error = nullptr);
};

#endif
