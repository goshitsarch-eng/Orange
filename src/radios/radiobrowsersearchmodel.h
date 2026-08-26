#ifndef STRAWBERRY_RADIOBROWSERSEARCHMODEL_H
#define STRAWBERRY_RADIOBROWSERSEARCHMODEL_H

#include "radios/radiochannel.h"

#include <string>
#include <vector>

class RadioBrowserSearchModel {
 public:
  enum class Column { Name, Country, Tags, Codec };

  static constexpr int kColumnCount = 4;

  void SetResults(const std::vector<RadioChannel> &results) { results_ = results; }
  void Clear() { results_.clear(); }
  void AddChannels(const std::vector<RadioChannel> &channels) {
    results_.insert(results_.end(), channels.begin(), channels.end());
  }
  const std::vector<RadioChannel> &results() const { return results_; }
  int row_count() const { return static_cast<int>(results_.size()); }
  RadioChannel ChannelForRow(int row) const {
    if (row < 0 || row >= row_count()) {
      return RadioChannel();
    }
    return results_[static_cast<size_t>(row)];
  }

  static const char *ColumnLabel(Column column) {
    switch (column) {
      case Column::Country:
        return "Country";
      case Column::Tags:
        return "Tags";
      case Column::Codec:
        return "Codec";
      case Column::Name:
      default:
        return "Name";
    }
  }

  static std::string CellText(const RadioChannel &channel, Column column) {
    switch (column) {
      case Column::Country:
        return channel.country;
      case Column::Tags:
        return channel.tags;
      case Column::Codec:
        return channel.codec;
      case Column::Name:
      default:
        return channel.name;
    }
  }

  static std::string RowSummary(const RadioChannel &channel) {
    std::string text = channel.name;
    if (!channel.country.empty()) {
      text += " · " + channel.country;
    }
    if (!channel.codec.empty()) {
      text += " (" + channel.codec + ")";
    }
    if (!channel.tags.empty()) {
      text += " — " + channel.tags;
    }
    return text;
  }

 private:
  std::vector<RadioChannel> results_;
};

#endif
