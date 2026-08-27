#ifndef STRAWBERRY_DISKUTILS_H
#define STRAWBERRY_DISKUTILS_H

#include <cstdint>
#include <string>

namespace DiskUtils {

int64_t FreeSpaceBytes(const std::string &path);
int64_t TotalSpaceBytes(const std::string &path);

}  // namespace DiskUtils

#endif
