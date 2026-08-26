#include "globalshortcuts/globalshortcut.h"

GlobalShortcut::GlobalShortcut(std::string id, std::string description, std::string default_key)
    : id_(std::move(id)), description_(std::move(description)), default_key_(std::move(default_key)), key_(default_key_) {}
