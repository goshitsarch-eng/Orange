#ifndef STRAWBERRY_SCROBBLERSETTINGSSERVICE_H
#define STRAWBERRY_SCROBBLERSETTINGSSERVICE_H

#include <string>

class ScrobblerSettingsService {
 public:
  void Reload();
  bool enabled() const { return enabled_; }
  bool scrobble_offline() const { return scrobble_offline_; }
  int submit_delay() const { return submit_delay_; }

 private:
  bool enabled_ = false;
  bool scrobble_offline_ = true;
  int submit_delay_ = 0;
};

#endif
