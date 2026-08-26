#ifndef STRAWBERRY_RADIOBROWSERSEARCHMODEL_H
#define STRAWBERRY_RADIOBROWSERSEARCHMODEL_H

#include "radios/radiochannel.h"

#include <string>
#include <vector>

class RadioBrowserSearchModel {
 public:
  void SetResults(const std::vector<RadioChannel> &results) { results_ = results; }
  const std::vector<RadioChannel> &results() const { return results_; }
  int row_count() const { return static_cast<int>(results_.size()); }

 private:
  std::vector<RadioChannel> results_;
};

#endif
