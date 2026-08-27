#ifndef STRAWBERRY_ERRORDIALOGQUEUE_H
#define STRAWBERRY_ERRORDIALOGQUEUE_H

#include <string>
#include <vector>

namespace ErrorDialogQueue {

// Qt ErrorDialog::ShowMessage: ignore empty strings, then append.
inline bool ShouldEnqueue(const std::string &message) { return !message.empty(); }

inline bool Enqueue(std::vector<std::string> *messages, const std::string &message) {
  if (!messages || !ShouldEnqueue(message)) {
    return false;
  }
  messages->push_back(message);
  return true;
}

inline void Clear(std::vector<std::string> *messages) {
  if (messages) {
    messages->clear();
  }
}

inline std::string HtmlEscape(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (char ch : text) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out += ch;
        break;
    }
  }
  return out;
}

// Qt ErrorDialog::UpdateContent joins messages with <hr/> after HTML-escaping.
inline std::string JoinHtml(const std::vector<std::string> &messages) {
  std::string html;
  for (const std::string &message : messages) {
    if (!html.empty()) {
      html += "<hr/>";
    }
    html += HtmlEscape(message);
  }
  return html;
}

inline std::string JoinPlain(const std::vector<std::string> &messages) {
  std::string text;
  for (const std::string &message : messages) {
    if (!text.empty()) {
      text += "\n\n";
    }
    text += message;
  }
  return text;
}

// Qt ShowMessage: raise immediately when an active window exists; otherwise keep the dialog minimized.
inline bool ShouldShowNow(bool has_active_window) { return has_active_window; }

inline bool ShouldShowMinimized(bool has_active_window, bool already_visible) { return !has_active_window && !already_visible; }

// Qt MainWindow::CheckShowErrorDialog.
inline bool ShouldReraise(bool main_visible, bool main_minimized, bool dialog_visible, bool dialog_active) {
  return main_visible && !main_minimized && dialog_visible && !dialog_active;
}

}  // namespace ErrorDialogQueue

#endif
