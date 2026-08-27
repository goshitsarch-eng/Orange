#ifndef STRAWBERRY_STREAMINGSERVICES_H
#define STRAWBERRY_STREAMINGSERVICES_H

#include "core/network.h"
#include "core/song.h"
#include "core/taskmanager.h"
#include "core/urlhandlers.h"
#include "streaming/streamingservice.h"

#include <memory>
#include <string>
#include <vector>

class StreamingServices {
 public:
  StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers, TaskManager *task_manager = nullptr);
  std::vector<StreamingService *> All() const;
  StreamingService *ServiceByName(const std::string &name) const;

 private:
  std::vector<std::unique_ptr<StreamingService>> services_;
  std::vector<std::unique_ptr<UrlHandler>> handlers_;
};

#endif
