#include "covermanager/coversearchstatisticsdialog.h"

#include "covermanager/coversearchstatisticslabels.h"

#include <adwaita.h>

std::string CoverSearchStatisticsDialog::SummaryText(const CoverSearchStatistics &statistics) {
  return CoverSearchStatisticsLabels::Summary(statistics);
}

void CoverSearchStatisticsDialog::Show(GtkWindow *parent, const CoverSearchStatistics &statistics) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(CoverSearchStatisticsLabels::Title(), SummaryText(statistics).c_str()));
  adw_alert_dialog_add_responses(dialog, "close", "Close", nullptr);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
