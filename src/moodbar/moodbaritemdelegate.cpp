#include "moodbar/moodbaritemdelegate.h"

#include "constants/moodbarsettings.h"
#include "core/settings.h"
#include "core/standardpaths.h"

#include <glib.h>

#include <utility>

namespace {

constexpr int kMaxInflight = 2;

struct CellJob {
  MoodbarItemDelegate *delegate = nullptr;
  std::shared_ptr<bool> alive;
  Song song;
  std::string key;
  bool save = false;
  std::string cache_dir;
  std::vector<uint8_t> data;
};

}  // namespace

MoodbarItemDelegate::MoodbarItemDelegate() = default;

MoodbarItemDelegate::~MoodbarItemDelegate() { *alive_ = false; }

void MoodbarItemDelegate::Paint(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood) {
  const int inset = MoodbarCell::BorderInset();
  if (!cr || mood.empty() || width <= inset * 2 || height <= inset * 2) {
    return;
  }
  cairo_save(cr);
  cairo_translate(cr, inset, inset);
  MoodbarRenderer::Draw(cr, width - inset * 2, height - inset * 2, mood);
  cairo_restore(cr);
}

const std::vector<uint8_t> *MoodbarItemDelegate::Peek(const std::string &url) const {
  const auto it = cache_.find(url);
  if (it == cache_.end() || it->second.empty()) {
    return nullptr;
  }
  return &it->second;
}

const std::vector<uint8_t> &MoodbarItemDelegate::Ensure(const Song &song) {
  const std::string key = MoodbarCell::CacheKey(song);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    return it->second;
  }
  if (!MoodbarCell::CanLoad(song)) {
    return cache_[key];
  }
  std::vector<uint8_t> cached = loader_.LoadCached(song);
  if (!cached.empty()) {
    return cache_[key] = std::move(cached);
  }
  cache_[key] = {};
  if (!loading_[key]) {
    pending_.push_back(song);
    loading_[key] = true;
    MaybeStartNext();
  }
  return cache_[key];
}

void MoodbarItemDelegate::SetUpdatedCallback(const std::function<void()> &callback) { updated_ = callback; }

void MoodbarItemDelegate::FinishGenerate(const std::string &key, std::vector<uint8_t> data) {
  cache_[key] = std::move(data);
  loading_[key] = false;
  if (inflight_ > 0) {
    --inflight_;
  }
  MaybeStartNext();
  if (updated_) {
    updated_();
  }
}

void MoodbarItemDelegate::MaybeStartNext() {
  while (inflight_ < kMaxInflight && !pending_.empty()) {
    Song song = pending_.front();
    pending_.erase(pending_.begin());
    StartGenerate(song);
  }
}

void MoodbarItemDelegate::StartGenerate(const Song &song) {
  Settings settings;
  settings.BeginGroup(MoodbarSettings::kSettingsGroup);
  auto *job = new CellJob;
  job->delegate = this;
  job->alive = alive_;
  job->song = song;
  job->key = MoodbarCell::CacheKey(song);
  job->save = settings.BoolValue(MoodbarSettings::kSave, MoodbarSettings::kDefaultSave);
  job->cache_dir = StandardPaths::MoodbarCacheDir();
  ++inflight_;
  g_thread_unref(g_thread_new("moodbar-cell", +[](gpointer data) -> gpointer {
    auto *job = static_cast<CellJob *>(data);
    job->data = MoodbarLoader().Generate(job->song, job->save, job->cache_dir);
    g_idle_add(+[](gpointer idle) -> gboolean {
      std::unique_ptr<CellJob> job(static_cast<CellJob *>(idle));
      if (!job->alive || !*job->alive || !job->delegate) {
        return G_SOURCE_REMOVE;
      }
      job->delegate->FinishGenerate(job->key, std::move(job->data));
      return G_SOURCE_REMOVE;
    }, job);
    return nullptr;
  }, job));
}
