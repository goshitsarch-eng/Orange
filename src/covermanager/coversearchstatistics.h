#ifndef STRAWBERRY_COVERSEARCHSTATISTICS_H
#define STRAWBERRY_COVERSEARCHSTATISTICS_H

#include <cstdint>
#include <map>
#include <string>

struct CoverSearchStatistics {
  CoverSearchStatistics() = default;

  CoverSearchStatistics &operator+=(const CoverSearchStatistics &other);

  std::string AverageDimensions() const;

  uint64_t network_requests_made = 0;
  uint64_t bytes_transferred = 0;
  std::map<std::string, uint64_t> total_images_by_provider;
  std::map<std::string, uint64_t> chosen_images_by_provider;

  uint64_t chosen_images = 0;
  uint64_t missing_images = 0;

  uint64_t chosen_width = 0;
  uint64_t chosen_height = 0;
};

#endif
