#ifndef STRAWBERRY_STATUSBARSTACK_H
#define STRAWBERRY_STATUSBARSTACK_H

namespace StatusBarStack {

enum class Page { Loading, Summary };

inline Page PageForTaskCount(int tasks) { return tasks > 0 ? Page::Loading : Page::Summary; }

inline const char *ChildName(Page page) { return page == Page::Loading ? "loading" : "summary"; }

inline bool ShowLoading(int task_count) { return PageForTaskCount(task_count) == Page::Loading; }

}  // namespace StatusBarStack

#endif  // STRAWBERRY_STATUSBARSTACK_H
