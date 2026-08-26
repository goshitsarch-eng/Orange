#include "loginstatewidget.h"

LoginStateWidget::LoginStateWidget() {
  root_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  status_ = gtk_label_new("Not signed in");
  gtk_widget_set_halign(status_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(root_), status_);

  account_ = gtk_label_new("");
  gtk_widget_set_halign(account_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(root_), account_);

  account_type_ = gtk_label_new("");
  gtk_widget_set_halign(account_type_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(root_), account_type_);

  progress_ = gtk_spinner_new();
  gtk_box_append(GTK_BOX(root_), progress_);

  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  login_ = gtk_button_new_with_label("Log in");
  logout_ = gtk_button_new_with_label("Log out");
  gtk_box_append(GTK_BOX(buttons), login_);
  gtk_box_append(GTK_BOX(buttons), logout_);
  gtk_box_append(GTK_BOX(root_), buttons);

  g_signal_connect(login_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer self) {
                     auto *w = static_cast<LoginStateWidget *>(self);
                     if (w->login_cb_) w->login_cb_();
                   }),
                   this);
  g_signal_connect(logout_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer self) {
                     auto *w = static_cast<LoginStateWidget *>(self);
                     if (w->logout_cb_) w->logout_cb_();
                   }),
                   this);

  ApplyState();
}

LoginStateWidget::~LoginStateWidget() { root_ = nullptr; }

void LoginStateWidget::SetLoggedIn(State state, const std::string &account_name) {
  state_ = state;
  account_name_ = account_name;
  ApplyState();
}

void LoginStateWidget::AddCredentialField(GtkWidget *widget) {
  if (widget) gtk_box_append(GTK_BOX(root_), widget);
}

void LoginStateWidget::AddCredentialGroup(GtkWidget *widget) {
  AddCredentialField(widget);
}

void LoginStateWidget::HideExpires() {}

void LoginStateWidget::SetAccountTypeVisible(bool visible) {
  gtk_widget_set_visible(account_type_, visible);
}

void LoginStateWidget::ApplyState() {
  switch (state_) {
    case State::LoggedIn:
      gtk_label_set_text(GTK_LABEL(status_), "Signed in");
      gtk_label_set_text(GTK_LABEL(account_), account_name_.c_str());
      gtk_widget_set_visible(login_, FALSE);
      gtk_widget_set_visible(logout_, TRUE);
      gtk_spinner_stop(GTK_SPINNER(progress_));
      gtk_widget_set_visible(progress_, FALSE);
      break;
    case State::LoginInProgress:
      gtk_label_set_text(GTK_LABEL(status_), "Signing in…");
      gtk_widget_set_visible(login_, FALSE);
      gtk_widget_set_visible(logout_, FALSE);
      gtk_widget_set_visible(progress_, TRUE);
      gtk_spinner_start(GTK_SPINNER(progress_));
      break;
    case State::LoggedOut:
      gtk_label_set_text(GTK_LABEL(status_), "Not signed in");
      gtk_label_set_text(GTK_LABEL(account_), "");
      gtk_widget_set_visible(login_, TRUE);
      gtk_widget_set_visible(logout_, FALSE);
      gtk_spinner_stop(GTK_SPINNER(progress_));
      gtk_widget_set_visible(progress_, FALSE);
      break;
  }
}
