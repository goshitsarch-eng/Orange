#include "organize/organizeformatvalidator.h"

#include "organize/organizeformat.h"

bool OrganizeFormatValidator::IsValid(const std::string &format, std::string *error) {
  OrganizeFormat parsed(format);
  if (parsed.IsValid()) {
    return true;
  }
  if (error) {
    *error = format.empty() ? "Format is empty" : "Unbalanced braces in format";
  }
  return false;
}
