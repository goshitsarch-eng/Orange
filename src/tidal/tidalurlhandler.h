#ifndef STRAWBERRY_TIDALURLHANDLER_H
#define STRAWBERRY_TIDALURLHANDLER_H

#include "core/urlhandlers.h"

class TaskManager;
class TidalService;

class TidalUrlHandler : public UrlHandler {
 public:
  TidalUrlHandler(TidalService *service, TaskManager *task_manager = nullptr);

  std::string scheme() const override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;

 private:
  TidalService *service_ = nullptr;
  TaskManager *task_manager_ = nullptr;
};

#endif
