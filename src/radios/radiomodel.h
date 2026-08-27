#ifndef STRAWBERRY_RADIOMODEL_H
#define STRAWBERRY_RADIOMODEL_H

#include "radios/radiochannel.h"
#include "radios/radiotree.h"

#include <set>
#include <string>
#include <vector>

class RadioModel {
 public:
  void SetChannels(const std::vector<RadioChannel> &channels);
  void SetSearchResults(const std::vector<RadioChannel> &channels);
  const std::vector<RadioChannel> &visible() const { return visible_; }
  std::string Label(const RadioChannel &channel) const;
  int row_count() const { return static_cast<int>(visible_.size()); }
  std::vector<RadioTree::Row> Rows() const { return RadioTree::VisibleRows(visible_, collapsed_); }
  bool Toggle(Song::Source source) { return RadioTree::Toggle(&collapsed_, source); }
  bool Expanded(Song::Source source) const { return RadioTree::ShowChildren(source, collapsed_); }
  const std::set<Song::Source> &collapsed() const { return collapsed_; }

 private:
  std::vector<RadioChannel> visible_;
  std::set<Song::Source> collapsed_;
};

#endif
