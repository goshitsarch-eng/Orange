#include "covermanager/coversearchstatisticsdialog.h"

#include "utilities/fileutils.h"

#include <adwaita.h>

#include <sstream>

std::string CoverSearchStatisticsDialog::SummaryText(const CoverSearchStatistics &statistics) {
  std::ostringstream out;
  out << "Network requests: " << statistics.network_requests_made << "\n"
      << "Bytes transferred: " << FileUtils::PrettySize(static_cast<int64_t>(statistics.bytes_transferred)) << "\n"
      << "Chosen images: " << statistics.chosen_images << "\n"
      << "Missing images: " << statistics.missing_images << "\n"
      << "Average size: " << statistics.AverageDimensions() << "\n";
  if (!statistics.total_images_by_provider.empty()) {
    out << "\nBy provider:\n";
    for (const auto &entry : statistics.total_images_by_provider) {
      const uint64_t chosen = statistics.chosen_images_by_provider.count(entry.first) ? statistics.chosen_images_by_provider.at(entry.first) : 0;
      out << "  " << entry.first << ": " << entry.second << " found, " << chosen << " chosen\n";
    }
  }
  return out.str();
}

void CoverSearchStatisticsDialog::Show(GtkWindow *parent, const CoverSearchStatistics &statistics) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Cover search statistics", SummaryText(statistics).c_str()));
  adw_alert_dialog_add_responses(dialog, "close", "Close", nullptr);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
