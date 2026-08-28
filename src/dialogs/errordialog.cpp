#include "dialogs/errordialog.h"

#include "dialogs/dialogclosekeys.h"
#include "dialogs/errordialoglabels.h"
#include "dialogs/errordialogqueue.h"
#include "translations/translations.h"

#include <adwaita.h>

void ErrorDialog::Show(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(ErrorDialogLabels::Title()), message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", Translations::CStr("OK"));
  adw_alert_dialog_set_default_response(dialog, "ok");
  adw_alert_dialog_set_close_response(dialog, "ok");
  DialogCloseKeys::Attach(ADW_DIALOG(dialog));
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

QueuedErrorDialog::QueuedErrorDialog(GtkWindow *parent) : parent_(parent) {}

QueuedErrorDialog::~QueuedErrorDialog() {
  if (dialog_) {
    g_signal_handlers_disconnect_by_data(dialog_, this);
    dialog_ = nullptr;
  }
}

void QueuedErrorDialog::ShowMessage(const std::string &message) {
  if (!ErrorDialogQueue::Enqueue(&messages_, message)) {
    return;
  }
  EnsureDialog();
  RefreshBody();
  if (ErrorDialogQueue::ShouldShowNow(parent_ && gtk_widget_get_mapped(GTK_WIDGET(parent_)))) {
    Present();
  }
}

void QueuedErrorDialog::Present() {
  if (messages_.empty() || !parent_) {
    return;
  }
  EnsureDialog();
  RefreshBody();
  adw_dialog_present(ADW_DIALOG(dialog_), GTK_WIDGET(parent_));
  visible_ = true;
  active_ = true;
}

void QueuedErrorDialog::EnsureDialog() {
  if (dialog_) {
    return;
  }
  dialog_ = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(ErrorDialogLabels::Title()), ""));
  adw_alert_dialog_add_response(dialog_, "ok", Translations::CStr("OK"));
  adw_alert_dialog_set_default_response(dialog_, "ok");
  adw_alert_dialog_set_close_response(dialog_, "ok");
  DialogCloseKeys::Attach(ADW_DIALOG(dialog_));
  g_signal_connect(dialog_, "closed", G_CALLBACK((+[](AdwDialog *, gpointer data) {
                     static_cast<QueuedErrorDialog *>(data)->OnClosed();
                   })),
                   this);
}

void QueuedErrorDialog::RefreshBody() {
  if (dialog_) {
    adw_alert_dialog_set_body(dialog_, ErrorDialogQueue::JoinPlain(messages_).c_str());
  }
}

void QueuedErrorDialog::OnClosed() {
  ErrorDialogQueue::Clear(&messages_);
  dialog_ = nullptr;
  visible_ = false;
  active_ = false;
}
