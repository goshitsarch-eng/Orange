#ifndef STRAWBERRY_QOBUZURLHANDLER_H
#define STRAWBERRY_QOBUZURLHANDLER_H

#include "core/urlhandlers.h"

class TaskManager;
class QobuzService;

class QobuzUrlHandler : public UrlHandler {
 public:
  QobuzUrlHandler(QobuzService *service, TaskManager *task_manager = nullptr);

  std::string scheme() const override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;

 private:
  QobuzService *service_ = nullptr;
  TaskManager *task_manager_ = nullptr;
};

#endif
