#include "transcoder/transcodedialog.h"

#include "constants/transcodersettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "transcoder/transcodeui.h"
#include "transcoder/transcodelog.h"
#include "transcoder/transcodelogdialog.h"
#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsdialog.h"
#include "translations/translations.h"
#include "utilities/filefilters.h"
#include "utilities/fileutils.h"

#include <adwaita.h>
#include <glib/gstdio.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {

struct State {
  Application *app = nullptr;
  GtkWindow *parent = nullptr;
  AdwDialog *dialog = nullptr;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  std::vector<TranscodeUi::QueueItem> files;
  GtkWidget *list = nullptr;
  GtkWidget *formats = nullptr;
  GtkWidget *quality = nullptr;
  GtkWidget *dest = nullptr;
  GtkWidget *preserve = nullptr;
  GtkWidget *progress = nullptr;
  GtkWidget *status = nullptr;
  GtkWidget *log = nullptr;
  GtkWidget *details = nullptr;
  GtkWidget *log_view = nullptr;
  std::vector<std::string> log_lines;
  GtkWidget *start = nullptr;
  GtkWidget *cancel = nullptr;
  GtkWidget *add = nullptr;
  GtkWidget *import = nullptr;
  GtkWidget *remove = nullptr;
  GtkWidget *options = nullptr;
  GtkWidget *browse = nullptr;
  bool working = false;
  int remaining = 0;
  int success = 0;
  int failed = 0;

  ~State() { *alive = false; }
};

struct ChooserJob {
  std::shared_ptr<bool> alive;
  State *state = nullptr;
};

void RefreshList(State *state) {
  if (!state || !state->list) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(state->list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(state->list), child);
    child = next;
  }
  for (const TranscodeUi::QueueItem &item : state->files) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(item.path.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    gtk_list_box_append(GTK_LIST_BOX(state->list), row);
  }
}

void SetWorking(State *state, bool working) {
  if (!state) {
    return;
  }
  state->working = working;
  const gboolean sensitive = working ? FALSE : TRUE;
  if (state->start) {
    gtk_widget_set_visible(state->start, TranscodeUi::ShouldShowCancel(working) ? FALSE : TRUE);
  }
  if (state->cancel) {
    gtk_widget_set_visible(state->cancel, TranscodeUi::ShouldShowCancel(working) ? TRUE : FALSE);
  }
  for (GtkWidget *widget : {state->add, state->import, state->remove, state->formats, state->quality, state->dest, state->preserve,
                            state->options, state->browse, state->list}) {
    if (widget) {
      gtk_widget_set_sensitive(widget, sensitive);
    }
  }
  if (state->dialog) {
    adw_dialog_set_can_close(state->dialog, working ? FALSE : TRUE);
  }
}

void RefreshLogView(State *state) {
  if (!state || !state->log_view) {
    return;
  }
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->log_view)), TranscodeLog::Join(state->log_lines).c_str(), -1);
}

void RecordLog(State *state, const std::string &message) {
  if (!state) {
    return;
  }
  TranscodeLog::Append(&state->log_lines, TranscodeLog::FormatLine(TranscodeLog::NowStamp(), message));
  if (state->log) {
    gtk_label_set_text(GTK_LABEL(state->log), TranscodeLog::LastLine(state->log_lines).c_str());
  }
  RefreshLogView(state);
}

void UpdateProgress(State *state) {
  if (!state) {
    return;
  }
  const int total = state->remaining + state->success + state->failed;
  const int value = TranscodeUi::ProgressBarValue(state->success, state->failed, total, {});
  if (state->progress) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(state->progress), TranscodeUi::ProgressFraction(value, total));
  }
  if (state->status) {
    gtk_label_set_text(GTK_LABEL(state->status), TranscodeUi::StatusText(state->remaining, state->success, state->failed).c_str());
  }
}

void AddPath(State *state, const std::string &path, const std::string &import_root) {
  if (!state || path.empty() || TranscodeUi::AlreadyQueued(state->files, path)) {
    return;
  }
  state->files.push_back({path, import_root});
}

void Persist(State *state) {
  if (!state) {
    return;
  }
  Settings settings;
  settings.BeginGroup(TranscoderSettings::kSettingsGroup);
  if (state->formats) {
    settings.SetValue(TranscoderSettings::kLastOutputFormat,
                      TranscodeUi::FormatKey(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(state->formats)))));
  }
  if (state->dest) {
    settings.SetValue(TranscoderSettings::kLastDestDir, gtk_editable_get_text(GTK_EDITABLE(state->dest)));
  }
  if (state->preserve) {
    settings.SetBoolValue(TranscoderSettings::kPreserveDirStructure, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->preserve)));
  }
  settings.Sync();
}

void StartJobs(State *state) {
  if (!state || !state->app || !state->app->transcoder() || state->working) {
    return;
  }
  Transcoder *transcoder = state->app->transcoder();
  transcoder->Cancel();
  transcoder->Progress.Clear();
  transcoder->Finished.Clear();
  transcoder->LogLine.Clear();
  const auto format = static_cast<Transcoder::Format>(gtk_drop_down_get_selected(GTK_DROP_DOWN(state->formats)));
  const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(state->dest));
  const bool preserve = gtk_check_button_get_active(GTK_CHECK_BUTTON(state->preserve));
  transcoder->set_quality(static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(state->quality))));
  std::vector<TranscodeUi::QueueItem> source = state->files;
  if (source.empty() && state->app->playlist_manager() && state->app->playlist_manager()->current()) {
    for (const Song &song : state->app->playlist_manager()->current()->songs()) {
      const std::string path = FileUtils::PathFromUri(song.url());
      if (!path.empty()) {
        source.push_back({path, {}});
      }
    }
  }
  for (const TranscodeUi::QueueItem &item : source) {
    Song song;
    song.set_url(FileUtils::UriFromPath(item.path));
    const std::string output = TranscodeUi::UniqueOutputPath(
        TranscodeUi::OutputPath(item.path, dest_dir, preserve, item.import_root, Transcoder::Extension(format)), FileUtils::Exists);
    if (output.empty()) {
      continue;
    }
    g_mkdir_with_parents(FileUtils::DirName(output).c_str(), 0755);
    transcoder->AddJob(song, output, format);
  }
  state->remaining = transcoder->job_count();
  state->success = 0;
  state->failed = 0;
  if (state->remaining <= 0) {
    if (state->log) {
      gtk_label_set_text(GTK_LABEL(state->log), Translations::CStr("No files to transcode"));
    }
    return;
  }
  const std::shared_ptr<bool> alive = state->alive;
  transcoder->Progress.Connect([alive, state, transcoder](int done, int total) {
    if (!*alive) {
      return;
    }
    state->success = transcoder->finished_success();
    state->failed = transcoder->finished_failed();
    state->remaining = std::max(0, total - done);
    UpdateProgress(state);
  });
  transcoder->LogLine.Connect([alive, state](const std::string &line) {
    if (!*alive) {
      return;
    }
    RecordLog(state, line);
  });
  Persist(state);
  SetWorking(state, true);
  UpdateProgress(state);
  transcoder->Start();
  if (*alive) {
    state->success = transcoder->finished_success();
    state->failed = transcoder->finished_failed();
    state->remaining = 0;
    UpdateProgress(state);
    SetWorking(state, false);
    if (state->log) {
      gtk_label_set_text(GTK_LABEL(state->log), TranscodeLog::LastLine(state->log_lines).c_str());
    }
    RefreshLogView(state);
  }
  transcoder->Progress.Clear();
  transcoder->Finished.Clear();
  transcoder->LogLine.Clear();
}

void OpenAddFiles(State *state) {
  if (!state || state->working) {
    return;
  }
  GtkFileDialog *chooser = gtk_file_dialog_new();
  gtk_file_dialog_set_title(chooser, Translations::CStr("Add files to transcode"));
  FileFilters::Apply(chooser, FileFilters::MediaFilters());
  Settings settings;
  settings.BeginGroup(TranscoderSettings::kSettingsGroup);
  const std::string last = settings.Value(TranscoderSettings::kLastAddDir);
  if (!last.empty()) {
    GFile *initial = g_file_new_for_path(last.c_str());
    gtk_file_dialog_set_initial_folder(chooser, initial);
    g_object_unref(initial);
  }
  auto *job = new ChooserJob{state->alive, state};
  gtk_file_dialog_open_multiple(chooser, state->parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    std::unique_ptr<ChooserJob> job(static_cast<ChooserJob *>(data));
    GError *error = nullptr;
    GListModel *files = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!files) {
      if (error) {
        g_error_free(error);
      }
      return;
    }
    if (!*job->alive || !job->state) {
      g_object_unref(files);
      return;
    }
    const guint n = g_list_model_get_n_items(files);
    std::string first;
    for (guint i = 0; i < n; ++i) {
      GFile *file = G_FILE(g_list_model_get_item(files, i));
      gchar *path = g_file_get_path(file);
      if (path && *path) {
        AddPath(job->state, path, {});
        if (first.empty()) {
          first = FileUtils::DirName(path);
        }
      }
      g_free(path);
      g_object_unref(file);
    }
    g_object_unref(files);
    RefreshList(job->state);
    if (!first.empty()) {
      Settings save;
      save.BeginGroup(TranscoderSettings::kSettingsGroup);
      save.SetValue(TranscoderSettings::kLastAddDir, first);
      save.Sync();
    }
  }, job);
}

void OpenImportFolder(State *state) {
  if (!state || state->working) {
    return;
  }
  GtkFileDialog *chooser = gtk_file_dialog_new();
  gtk_file_dialog_set_title(chooser, Translations::CStr("Open a directory to import music from"));
  Settings settings;
  settings.BeginGroup(TranscoderSettings::kSettingsGroup);
  const std::string last = settings.Value(TranscoderSettings::kLastImportDir);
  if (!last.empty()) {
    GFile *initial = g_file_new_for_path(last.c_str());
    gtk_file_dialog_set_initial_folder(chooser, initial);
    g_object_unref(initial);
  }
  auto *job = new ChooserJob{state->alive, state};
  gtk_file_dialog_select_folder(chooser, state->parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    std::unique_ptr<ChooserJob> job(static_cast<ChooserJob *>(data));
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) {
        g_error_free(error);
      }
      return;
    }
    gchar *path = g_file_get_path(file);
    g_object_unref(file);
    if (!path || !*job->alive || !job->state) {
      g_free(path);
      return;
    }
    const std::string root = path;
    g_free(path);
    for (const std::string &entry : FileUtils::ListDirectoryRecursive(root)) {
      if (TranscodeUi::IsAudioPath(entry)) {
        AddPath(job->state, entry, root);
      }
    }
    RefreshList(job->state);
    Settings save;
    save.BeginGroup(TranscoderSettings::kSettingsGroup);
    save.SetValue(TranscoderSettings::kLastImportDir, root);
    save.Sync();
  }, job);
}

void OpenDestination(State *state) {
  if (!state || state->working) {
    return;
  }
  GtkFileDialog *chooser = gtk_file_dialog_new();
  gtk_file_dialog_set_title(chooser, Translations::CStr("Destination folder"));
  const char *current = gtk_editable_get_text(GTK_EDITABLE(state->dest));
  if (current && *current) {
    GFile *initial = g_file_new_for_path(current);
    gtk_file_dialog_set_initial_folder(chooser, initial);
    g_object_unref(initial);
  }
  auto *job = new ChooserJob{state->alive, state};
  gtk_file_dialog_select_folder(chooser, state->parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    std::unique_ptr<ChooserJob> job(static_cast<ChooserJob *>(data));
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!file) {
      if (error) {
        g_error_free(error);
      }
      return;
    }
    gchar *path = g_file_get_path(file);
    g_object_unref(file);
    if (path && *job->alive && job->state && job->state->dest) {
      gtk_editable_set_text(GTK_EDITABLE(job->state->dest), path);
      Settings save;
      save.BeginGroup(TranscoderSettings::kSettingsGroup);
      save.SetValue(TranscoderSettings::kLastDestDir, path);
      save.Sync();
    }
    g_free(path);
  }, job);
}

void RemoveSelected(State *state) {
  if (!state || state->working || !state->list) {
    return;
  }
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->list));
  if (!row) {
    return;
  }
  const int index = gtk_list_box_row_get_index(row);
  if (index < 0 || static_cast<size_t>(index) >= state->files.size()) {
    return;
  }
  state->files.erase(state->files.begin() + index);
  RefreshList(state);
}

}  // namespace

void TranscodeDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Transcode Music"));
  adw_dialog_set_content_width(dialog, 640);
  adw_dialog_set_content_height(dialog, 620);

  auto *state = new State();
  state->app = app;
  state->parent = parent;
  state->dialog = dialog;
  g_object_set_data_full(G_OBJECT(dialog), "state", state, [](gpointer p) { delete static_cast<State *>(p); });

  Settings settings;
  settings.BeginGroup(TranscoderSettings::kSettingsGroup);
  const std::string music_dir = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)
                                                                              : g_get_home_dir();
  const std::string dest_dir = settings.Value(TranscoderSettings::kLastDestDir, music_dir);
  const int format_index = TranscodeUi::FormatIndexFromKey(settings.Value(TranscoderSettings::kLastOutputFormat, TranscoderSettings::kDefaultLastOutputFormat));
  const bool preserve = settings.BoolValue(TranscoderSettings::kPreserveDirStructure, false);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  GtkWidget *queue_header = gtk_label_new(Translations::CStr("Files"));
  gtk_widget_set_halign(queue_header, GTK_ALIGN_START);
  gtk_widget_add_css_class(queue_header, "heading");
  gtk_box_append(GTK_BOX(box), queue_header);

  state->list = gtk_list_box_new();
  gtk_widget_add_css_class(state->list, "boxed-list");
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 160);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), state->list);
  gtk_box_append(GTK_BOX(box), scroll);

  GtkWidget *queue_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  state->add = gtk_button_new_with_label(Translations::CStr("Add files…"));
  state->import = gtk_button_new_with_label(Translations::CStr("Import folder…"));
  state->remove = gtk_button_new_with_label(Translations::CStr("Remove"));
  gtk_box_append(GTK_BOX(queue_buttons), state->add);
  gtk_box_append(GTK_BOX(queue_buttons), state->import);
  gtk_box_append(GTK_BOX(queue_buttons), state->remove);
  gtk_box_append(GTK_BOX(box), queue_buttons);

  static const char *format_names[] = {"MP3", "AAC", "FLAC", "Ogg Vorbis", "Opus", "Speex", "WavPack", "ASF", nullptr};
  state->formats = gtk_drop_down_new_from_strings(format_names);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(state->formats), static_cast<guint>(format_index));
  state->quality = gtk_spin_button_new_with_range(0, 10, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->quality), 5);
  state->options = gtk_button_new_with_label(Translations::CStr("Format options…"));
  state->dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(state->dest), dest_dir.c_str());
  state->browse = gtk_button_new_with_label(Translations::CStr("Browse…"));
  state->preserve = gtk_check_button_new_with_label(Translations::CStr("Preserve directory structure"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->preserve), preserve ? TRUE : FALSE);

  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Output format")));
  gtk_box_append(GTK_BOX(box), state->formats);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Quality")));
  gtk_box_append(GTK_BOX(box), state->quality);
  gtk_box_append(GTK_BOX(box), state->options);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Destination folder")));
  GtkWidget *dest_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_hexpand(state->dest, TRUE);
  gtk_box_append(GTK_BOX(dest_row), state->dest);
  gtk_box_append(GTK_BOX(dest_row), state->browse);
  gtk_box_append(GTK_BOX(box), dest_row);
  gtk_box_append(GTK_BOX(box), state->preserve);

  state->progress = gtk_progress_bar_new();
  state->status = gtk_label_new("");
  gtk_widget_set_halign(state->status, GTK_ALIGN_START);
  gtk_widget_set_hexpand(state->status, TRUE);
  state->details = gtk_button_new_with_label(Translations::CStr("Details…"));
  GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(status_row), state->status);
  gtk_box_append(GTK_BOX(status_row), state->details);
  state->log = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(state->log), TRUE);
  gtk_widget_set_halign(state->log, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), state->progress);
  gtk_box_append(GTK_BOX(box), status_row);
  gtk_box_append(GTK_BOX(box), state->log);

  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(actions, GTK_ALIGN_END);
  state->cancel = gtk_button_new_with_label(Translations::CStr("Cancel"));
  state->start = gtk_button_new_with_label(Translations::CStr("Start"));
  gtk_widget_add_css_class(state->start, "suggested-action");
  gtk_box_append(GTK_BOX(actions), state->cancel);
  gtk_box_append(GTK_BOX(actions), state->start);
  gtk_box_append(GTK_BOX(box), actions);
  gtk_widget_set_visible(state->cancel, FALSE);

  for (const Song &song : songs) {
    AddPath(state, FileUtils::PathFromUri(song.url()), {});
  }
  RefreshList(state);

  g_signal_connect(state->add, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { OpenAddFiles(static_cast<State *>(data)); }), state);
  g_signal_connect(state->import, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { OpenImportFolder(static_cast<State *>(data)); }), state);
  g_signal_connect(state->remove, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { RemoveSelected(static_cast<State *>(data)); }), state);
  g_signal_connect(state->browse, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { OpenDestination(static_cast<State *>(data)); }), state);
  g_signal_connect(state->options, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     const auto format = static_cast<Transcoder::Format>(gtk_drop_down_get_selected(GTK_DROP_DOWN(self->formats)));
                     TranscoderOptionsDialog::Show(self->parent, format, [self](int value) {
                       if (self->quality) {
                         gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->quality), value);
                       }
                     });
                   }),
                   state);
  g_signal_connect(state->details, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     TranscodeLogDialog::Show(self->parent, &self->log_lines, &self->log_view, self->log);
                   }),
                   state);
  g_signal_connect(state->start, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { StartJobs(static_cast<State *>(data)); }), state);
  g_signal_connect(state->cancel, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *self = static_cast<State *>(data);
                     if (self->app && self->app->transcoder()) {
                       self->app->transcoder()->Cancel();
                     }
                   }),
                   state);

  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
