#ifndef STRAWBERRY_COVERSEARCHSTATISTICSLABELS_H
#define STRAWBERRY_COVERSEARCHSTATISTICSLABELS_H

#include "covermanager/coversearchstatistics.h"
#include "utilities/fileutils.h"

#include <string>

namespace CoverSearchStatisticsLabels {

inline const char *Title() { return "Fetch completed"; }
inline const char *Requests() { return "Total network requests made"; }
inline const char *AverageSize() { return "Average image size"; }
inline const char *Bytes() { return "Total bytes transferred"; }

inline std::string Got(uint64_t chosen, uint64_t missing) {
  return "Got " + std::to_string(chosen) + " covers out of " + std::to_string(chosen + missing) + " (" + std::to_string(missing) +
         " failed)";
}

inline std::string CoversFrom(const std::string &provider) { return "Covers from " + provider; }

inline std::string BytesValue(uint64_t bytes) {
  if (bytes == 0) {
    return "0 bytes";
  }
  return FileUtils::PrettySize(static_cast<int64_t>(bytes));
}

inline std::string Summary(const CoverSearchStatistics &statistics) {
  std::string out = Got(statistics.chosen_images, statistics.missing_images);
  for (const auto &entry : statistics.chosen_images_by_provider) {
    out += "\n" + CoversFrom(entry.first) + ": " + std::to_string(entry.second);
  }
  out += "\n" + std::string(Requests()) + ": " + std::to_string(statistics.network_requests_made);
  out += "\n" + std::string(AverageSize()) + ": " + statistics.AverageDimensions();
  out += "\n" + std::string(Bytes()) + ": " + BytesValue(statistics.bytes_transferred);
  return out;
}

}  // namespace CoverSearchStatisticsLabels

#endif
