#ifndef STRAWBERRY_SHORTCUTSCATALOG_H
#define STRAWBERRY_SHORTCUTSCATALOG_H

#include <string>
#include <vector>

namespace ShortcutsCatalog {

struct Entry {
  const char *keys = "";
  const char *action = "";
};

// Wired MainWindow accels plus Space/Left/Right from Qt MainWindow::keyPressEvent.
inline std::vector<Entry> Entries() {
  return {
      {"Space", "Play/Pause"},
      {"Left", "Seek backward"},
      {"Right", "Seek forward"},
      {"F5", "Previous track"},
      {"F6", "Play/Pause"},
      {"F7", "Stop"},
      {"F8", "Next track"},
      {"Ctrl+Z", "Undo"},
      {"Ctrl+Shift+Z", "Redo"},
      {"Ctrl+N", "New playlist"},
      {"Ctrl+O", "Open files"},
      {"Ctrl+Shift+A", "Add file"},
      {"Ctrl+S", "Save playlist"},
      {"Ctrl+Shift+O", "Load playlist"},
      {"Ctrl+K", "Clear playlist"},
      {"Ctrl+J", "Jump to playing track"},
      {"Ctrl+L", "Love"},
      {"Ctrl+E", "Edit tags"},
      {"Ctrl+T", "Auto-complete tags"},
      {"Ctrl+Shift+T", "Transcode selected"},
      {"F2", "Edit playlist value"},
      {"Ctrl+F", "Focus collection search"},
      {"Ctrl+D", "Queue selected tracks"},
      {"Ctrl+Shift+D", "Queue play next"},
      {"Ctrl+Tab", "Next playlist"},
      {"Ctrl+Page Down", "Next playlist"},
      {"Ctrl+Shift+Tab", "Previous playlist"},
      {"Ctrl+Page Up", "Previous playlist"},
      {"Ctrl+9", "Last playlist"},
      {"Ctrl+Shift+P", "Active playlist"},
      {"Ctrl+W", "Close playlist"},
      {"Ctrl+M", "Mute"},
      {"Ctrl+Alt+V", "Stop after this track"},
      {"Ctrl+Shift+H", "Shuffle playlist"},
      {"F1", "About"},
      {"Ctrl+Q", "Quit"},
      {"Ctrl+,", "Preferences"},
      {"Ctrl+P", "Preferences"},
  };
}

inline std::string Line(const Entry &entry) {
  std::string line = entry.keys ? entry.keys : "";
  line += "  ";
  line += entry.action ? entry.action : "";
  return line;
}

inline std::string Text() {
  std::string text;
  for (const Entry &entry : Entries()) {
    if (!text.empty()) {
      text += "\n";
    }
    text += Line(entry);
  }
  return text;
}

inline bool ContainsKeys(const std::string &text, const char *keys) {
  return keys && !std::string(keys).empty() && text.find(keys) != std::string::npos;
}

}  // namespace ShortcutsCatalog

#endif
