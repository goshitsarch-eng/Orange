#ifndef STRAWBERRY_IMAGEUTILS_H
#define STRAWBERRY_IMAGEUTILS_H

#include <string>
#include <vector>

namespace ImageUtils {

bool IsJpeg(const std::string &data);
bool IsPng(const std::string &data);
std::vector<unsigned char> ScaleIfNeeded(const std::vector<unsigned char> &data, int max_size);

}  // namespace ImageUtils

#endif
