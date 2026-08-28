#ifndef STRAWBERRY_STRUTILS_H
#define STRAWBERRY_STRUTILS_H

#include "core/song.h"

#include <string>
#include <vector>

namespace StrUtils {

std::string ToLower(const std::string &value);
std::string ToUpper(const std::string &value);
std::string Trim(const std::string &value);
std::vector<std::string> Split(const std::string &value, char delimiter);
std::string Join(const std::vector<std::string> &parts, const std::string &delimiter);
bool StartsWith(const std::string &value, const std::string &prefix);
bool EndsWith(const std::string &value, const std::string &suffix);
bool ContainsInsensitive(const std::string &haystack, const std::string &needle);
std::string Replace(const std::string &value, const std::string &from, const std::string &to);
std::string SqlLikeEscape(const std::string &value);
std::string SqlQuote(const std::string &value);
// Parses a complete decimal number, rejecting anything with trailing characters.
// Used to keep user-supplied filter values out of SQL unless they really are numbers.
bool ParseNumber(const std::string &value, double *out);
// Renders a number as a SQL numeric literal, never in exponent form and never locale-dependent.
std::string FormatNumberForSql(double value);
std::string UriEscape(const std::string &value);
std::string JsonEscape(const std::string &value);
std::string Transliterate(const std::string &value);
std::string ReplaceMessage(const std::string &message, const Song &song, const std::string &newline = "\n");

}  // namespace StrUtils

#endif
