#include "tidal/tidalurlhandler.h"

#include "core/taskmanager.h"
#include "tidal/tidalservice.h"

TidalUrlHandler::TidalUrlHandler(TidalService *service, TaskManager *task_manager) : service_(service), task_manager_(task_manager) {}

std::string TidalUrlHandler::scheme() const { return service_ ? service_->scheme() : "tidal"; }

UrlHandler::LoadResult TidalUrlHandler::Load(const std::string &url, AsyncCallback callback) {
  const int task_id = task_manager_ ? task_manager_->StartTask("Loading tidal stream...") : 0;
  auto finish = [this, callback, task_id](const LoadResult &result) {
    if (task_manager_ && task_id > 0) {
      task_manager_->SetTaskFinished(task_id);
    }
    if (callback) {
      callback(result);
    }
  };
  if (!service_) {
    LoadResult result;
    result.media_url = url;
    result.error = "Tidal is not available";
    finish(result);
    return result;
  }
  return service_->Load(url, std::move(finish));
}
