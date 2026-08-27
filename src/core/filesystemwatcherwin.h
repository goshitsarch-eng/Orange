#ifndef STRAWBERRY_FILESYSTEMWATCHERWIN_H
#define STRAWBERRY_FILESYSTEMWATCHERWIN_H
#ifdef _WIN32
#include "core/filesystemwatcherinterface.h"
class FileSystemWatcherWin : public FileSystemWatcherInterface {};
#endif
#endif
