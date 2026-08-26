#ifndef LOGINSTATEWIDGET_H
#define LOGINSTATEWIDGET_H

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class LoginStateWidget {
 public:
  enum class State { LoggedIn, LoginInProgress, LoggedOut };

  explicit LoginStateWidget();
  ~LoginStateWidget();

  LoginStateWidget(const LoginStateWidget &) = delete;
  LoginStateWidget &operator=(const LoginStateWidget &) = delete;

  GtkWidget *widget() const { return root_; }

  void SetLoggedIn(State state, const std::string &account_name = {});
  State state() const { return state_; }
  std::string account_name() const { return account_name_; }

  void AddCredentialField(GtkWidget *widget);
  void AddCredentialGroup(GtkWidget *widget);

  void HideExpires();
  void SetAccountTypeVisible(bool visible);

  void SetLoginCallback(std::function<void()> callback) { login_cb_ = std::move(callback); }
  void SetLogoutCallback(std::function<void()> callback) { logout_cb_ = std::move(callback); }

 private:
  void ApplyState();
  void ApplyCredentials();

  GtkWidget *root_ = nullptr;
  GtkWidget *status_ = nullptr;
  GtkWidget *account_ = nullptr;
  GtkWidget *login_ = nullptr;
  GtkWidget *logout_ = nullptr;
  GtkWidget *progress_ = nullptr;
  GtkWidget *account_type_ = nullptr;
  GtkWidget *expires_ = nullptr;
  std::vector<GtkWidget *> credentials_;
  State state_ = State::LoggedOut;
  std::string account_name_;
  std::function<void()> login_cb_;
  std::function<void()> logout_cb_;
};

#endif  // LOGINSTATEWIDGET_H
