#include "covermanager/coversearchstatistics.h"

CoverSearchStatistics &CoverSearchStatistics::operator+=(const CoverSearchStatistics &other) {
  network_requests_made += other.network_requests_made;
  bytes_transferred += other.bytes_transferred;
  for (const auto &entry : other.chosen_images_by_provider) {
    chosen_images_by_provider[entry.first] += entry.second;
  }
  for (const auto &entry : other.total_images_by_provider) {
    total_images_by_provider[entry.first] += entry.second;
  }
  chosen_images += other.chosen_images;
  missing_images += other.missing_images;
  chosen_width += other.chosen_width;
  chosen_height += other.chosen_height;
  return *this;
}

std::string CoverSearchStatistics::AverageDimensions() const {
  if (chosen_images == 0) {
    return "0x0";
  }
  return std::to_string(chosen_width / chosen_images) + "x" + std::to_string(chosen_height / chosen_images);
}
