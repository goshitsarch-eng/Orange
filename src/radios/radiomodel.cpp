#include "radios/radiomodel.h"

void RadioModel::SetChannels(const std::vector<RadioChannel> &channels) { visible_ = channels; }

void RadioModel::SetSearchResults(const std::vector<RadioChannel> &channels) { visible_ = channels; }

std::string RadioModel::Label(const RadioChannel &channel) const {
  std::string label = channel.name.empty() ? channel.url : channel.name;
  if (!channel.country.empty()) {
    label += " · " + channel.country;
  }
  if (!channel.codec.empty()) {
    label += " (" + channel.codec + ")";
  }
  return label;
}
