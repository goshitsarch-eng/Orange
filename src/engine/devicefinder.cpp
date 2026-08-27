#include "engine/devicefinder.h"

DeviceFinder::DeviceFinder(std::string name, std::vector<std::string> outputs)
    : name_(std::move(name)), outputs_(std::move(outputs)) {}
