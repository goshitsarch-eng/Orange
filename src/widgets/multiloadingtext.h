#ifndef STRAWBERRY_MULTILOADINGTEXT_H
#define STRAWBERRY_MULTILOADINGTEXT_H

#include <string>
#include <vector>

namespace MultiLoadingText {

struct Task {
  std::string name;
  int progress = 0;
  int progress_max = 0;
};

inline std::string LowerFirst(std::string text) {
  if (!text.empty() && text[0] >= 'A' && text[0] <= 'Z') {
    text[0] = static_cast<char>(text[0] - 'A' + 'a');
  }
  return text;
}

inline std::string UpperFirst(std::string text) {
  if (!text.empty() && text[0] >= 'a' && text[0] <= 'z') {
    text[0] = static_cast<char>(text[0] - 'a' + 'A');
  }
  return text;
}

inline int Percent(int progress, int progress_max) {
  if (progress_max <= 0) {
    return 0;
  }
  return static_cast<int>(static_cast<float>(progress) / static_cast<float>(progress_max) * 100.0f);
}

inline std::string ItemText(const Task &task) {
  std::string text = LowerFirst(task.name);
  if (task.progress_max > 0) {
    text += " " + std::to_string(Percent(task.progress, task.progress_max)) + "%";
  }
  return text;
}

inline std::string Format(const std::vector<Task> &tasks) {
  if (tasks.empty()) {
    return {};
  }
  std::string text;
  for (size_t i = 0; i < tasks.size(); ++i) {
    if (i > 0) {
      text += ", ";
    }
    text += ItemText(tasks[i]);
  }
  return UpperFirst(text) + "...";
}

inline bool ShowIndicator(int task_count) { return task_count > 0; }

}  // namespace MultiLoadingText

namespace StatusBarStack {

inline bool ShowLoading(int task_count) { return MultiLoadingText::ShowIndicator(task_count); }

}  // namespace StatusBarStack

#endif  // STRAWBERRY_MULTILOADINGTEXT_H
