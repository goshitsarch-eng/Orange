#include "utilities/diskutils.h"

#include "utilities/fileutils.h"

namespace DiskUtils {

int64_t FreeSpaceBytes(const std::string &path) { return FileUtils::FreeSpaceBytes(path); }

int64_t TotalSpaceBytes(const std::string &path) { return FileUtils::TotalSpaceBytes(path); }

}  // namespace DiskUtils
