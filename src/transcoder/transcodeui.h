#ifndef STRAWBERRY_TRANSCODEUI_H
#define STRAWBERRY_TRANSCODEUI_H

#include <functional>
#include <string>
#include <vector>

namespace TranscodeUi {

struct QueueItem {
  std::string path;
  std::string import_root;
};

inline std::string JoinPath(const std::string &dir, const std::string &name) {
  if (dir.empty()) {
    return name;
  }
  if (dir.back() == '/') {
    return dir + name;
  }
  return dir + "/" + name;
}

inline std::string TrimBasename(const std::string &path) {
  std::string name = path;
  const std::string::size_type slash = name.find_last_of('/');
  if (slash != std::string::npos) {
    name = name.substr(slash + 1);
  }
  const std::string::size_type dot = name.rfind('.');
  if (dot != std::string::npos && dot > 0) {
    name = name.substr(0, dot);
  }
  return name;
}

inline std::string RelativeParent(const std::string &input, const std::string &import_root) {
  if (import_root.empty() || input.rfind(import_root, 0) != 0) {
    return {};
  }
  std::string rest = input.substr(import_root.size());
  if (!rest.empty() && rest[0] == '/') {
    rest = rest.substr(1);
  }
  const std::string::size_type slash = rest.find_last_of('/');
  if (slash == std::string::npos) {
    return {};
  }
  return rest.substr(0, slash);
}

inline std::string OutputPath(const std::string &input, const std::string &dest_dir, bool preserve_dirs, const std::string &import_root,
                             const std::string &extension) {
  const std::string stem = TrimBasename(input);
  if (stem.empty()) {
    return {};
  }
  if (dest_dir.empty()) {
    const std::string::size_type slash = input.find_last_of('/');
    const std::string dir = slash == std::string::npos ? std::string() : input.substr(0, slash);
    return JoinPath(dir, stem + "." + extension);
  }
  std::string dir = dest_dir;
  if (preserve_dirs) {
    const std::string relative = RelativeParent(input, import_root);
    if (!relative.empty()) {
      dir = JoinPath(dir, relative);
    }
  }
  return JoinPath(dir, stem + "." + extension);
}

inline std::string UniqueOutputPath(const std::string &path, const std::function<bool(const std::string &)> &exists) {
  if (!exists || !exists(path)) {
    return path;
  }
  const std::string::size_type slash = path.find_last_of('/');
  const std::string dir = slash == std::string::npos ? std::string() : path.substr(0, slash);
  const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
  const std::string::size_type dot = name.rfind('.');
  const std::string stem = (dot == std::string::npos || dot == 0) ? name : name.substr(0, dot);
  const std::string ext = (dot == std::string::npos || dot == 0) ? std::string() : name.substr(dot);
  for (int i = 1; i < 1000000; ++i) {
    const std::string candidate = JoinPath(dir, stem + "-" + std::to_string(i) + ext);
    if (!exists(candidate)) {
      return candidate;
    }
  }
  return path;
}

inline int ProgressBarMax(int total) { return total > 0 ? total * 100 : 1; }

inline int ProgressBarValue(int finished_ok, int finished_fail, int total, const std::vector<float> &active_fractions) {
  if (total <= 0) {
    return 0;
  }
  int progress = (finished_ok + finished_fail) * 100;
  for (float fraction : active_fractions) {
    if (fraction < 0.0f) {
      fraction = 0.0f;
    }
    if (fraction > 0.99f) {
      fraction = 0.99f;
    }
    progress += static_cast<int>(fraction * 100);
  }
  const int max = ProgressBarMax(total);
  return progress > max ? max : progress;
}

inline double ProgressFraction(int value, int total) {
  const int max = ProgressBarMax(total);
  return max > 0 ? static_cast<double>(value) / static_cast<double>(max) : 0.0;
}

inline std::string StatusText(int remaining, int success, int failed) {
  std::string text;
  auto append = [&text](const std::string &part) {
    if (!text.empty()) {
      text += ", ";
    }
    text += part;
  };
  if (remaining > 0) {
    append(std::to_string(remaining) + " remaining");
  }
  if (success > 0) {
    append(std::to_string(success) + " finished");
  }
  if (failed > 0) {
    append(std::to_string(failed) + " failed");
  }
  return text;
}

inline bool ShouldShowCancel(bool working) { return working; }

inline bool ShouldContinue(bool cancelled, int remaining) { return !cancelled && remaining > 0; }

inline bool AlreadyQueued(const std::vector<QueueItem> &files, const std::string &path) {
  for (const QueueItem &item : files) {
    if (item.path == path) {
      return true;
    }
  }
  return false;
}

inline bool IsAudioPath(const std::string &path) {
  const std::string::size_type dot = path.rfind('.');
  if (dot == std::string::npos || dot + 1 >= path.size()) {
    return false;
  }
  std::string ext = path.substr(dot + 1);
  for (char &ch : ext) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return ext == "mp3" || ext == "flac" || ext == "ogg" || ext == "oga" || ext == "opus" || ext == "m4a" || ext == "aac" ||
         ext == "wma" || ext == "wv" || ext == "wav" || ext == "spx" || ext == "ape" || ext == "mp4" || ext == "m4b";
}

inline int FormatIndexFromKey(const std::string &key) {
  if (key == "audio/mp4" || key == "aac") {
    return 1;
  }
  if (key == "audio/mpeg" || key == "mp3") {
    return 0;
  }
  if (key == "audio/x-flac" || key == "flac") {
    return 2;
  }
  if (key == "audio/x-opus" || key == "opus") {
    return 4;
  }
  if (key == "audio/x-speex" || key == "speex") {
    return 5;
  }
  if (key == "audio/x-wavpack" || key == "wavpack") {
    return 6;
  }
  if (key == "audio/x-wma" || key == "asf") {
    return 7;
  }
  return 3;
}

inline const char *FormatKey(int index) {
  static const char *kKeys[] = {"audio/mpeg", "audio/mp4", "audio/x-flac", "audio/x-vorbis",
                                "audio/x-opus", "audio/x-speex", "audio/x-wavpack", "audio/x-wma"};
  if (index < 0 || index > 7) {
    return kKeys[3];
  }
  return kKeys[index];
}

}  // namespace TranscodeUi

#endif
