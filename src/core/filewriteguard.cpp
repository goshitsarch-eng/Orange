#include "core/filewriteguard.h"

#include <unistd.h>

FileWriteGuard::FileWriteGuard(const std::string &path) : path_(path) { ok_ = !path.empty() && access(path.c_str(), W_OK) == 0; }

FileWriteGuard::~FileWriteGuard() = default;
