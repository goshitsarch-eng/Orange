#ifndef STRAWBERRY_COVERSEARCHSTATISTICSDIALOG_H
#define STRAWBERRY_COVERSEARCHSTATISTICSDIALOG_H

#include "covermanager/coversearchstatistics.h"

#include <gtk/gtk.h>

class CoverSearchStatisticsDialog {
 public:
  static void Show(GtkWindow *parent, const CoverSearchStatistics &statistics);
  static std::string SummaryText(const CoverSearchStatistics &statistics);
};

#endif
