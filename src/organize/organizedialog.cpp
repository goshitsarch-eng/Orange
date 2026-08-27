#include "organize/organizedialog.h"

#include "collection/collectionlibrary.h"
#include "constants/organizesettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "organize/organize.h"
#include "organize/organizejob.h"
#include "organize/organizepathnotify.h"
#include "core/standardpaths.h"
#include "organize/organizeerrordialog.h"
#include "organize/organizeformat.h"
#include "organize/organizeformatvalidator.h"
#include "organize/organizepreview.h"
#include "organize/organizesyntaxhighlighter.h"
#include "organize/organizetokenhelp.h"
#include "organize/organizetranscode.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "widgets/freespacebar.h"

#include <adwaita.h>

#include <memory>
#include <vector>

namespace {

struct DialogState {
  Application *app = nullptr;
  SongList songs;
  GtkTextBuffer *buffer = nullptr;
  GtkWidget *dest = nullptr;
  GtkWidget *remove_problematic = nullptr;
  GtkWidget *remove_non_fat = nullptr;
  GtkWidget *remove_non_ascii = nullptr;
  GtkWidget *allow_ascii_ext = nullptr;
  GtkWidget *replace_spaces = nullptr;
  GtkWidget *preview_list = nullptr;
  GtkWidget *run = nullptr;
  GtkWidget *cancel = nullptr;
  GtkWidget *status = nullptr;
  GtkWidget *after_copy = nullptr;
  GtkWidget *overwrite = nullptr;
  GtkWidget *albumcover = nullptr;
  GtkWidget *eject = nullptr;
  GtkWidget *format_error = nullptr;
  FreeSpaceBar *space = nullptr;
  Organize *job = nullptr;
  std::shared_ptr<bool> alive = std::make_shared<bool>(true);
  bool persist_dest = true;
  MusicStorage::TranscodeMode transcode_mode = MusicStorage::TranscodeMode::Transcode_Never;
  Song::FileType transcode_format = Song::FileType::Unknown;
  std::vector<Song::FileType> supported;

  ~DialogState() {
    if (alive) {
      *alive = false;
    }
    if (job) {
      job->Cancel();
    }
  }
};

OrganizeFormat FormatFromState(const DialogState *state) {
  OrganizeFormat format;
  if (state && state->buffer) {
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(state->buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(state->buffer, &start, &end, FALSE);
    format.set_format(text ? text : "");
    g_free(text);
  }
  if (state) {
    format.set_remove_problematic(state->remove_problematic && gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_problematic)));
    format.set_remove_non_fat(state->remove_non_fat && gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_non_fat)));
    format.set_remove_non_ascii(state->remove_non_ascii && gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_non_ascii)));
    format.set_allow_ascii_ext(state->allow_ascii_ext && gtk_check_button_get_active(GTK_CHECK_BUTTON(state->allow_ascii_ext)));
    format.set_replace_spaces(state->replace_spaces && gtk_check_button_get_active(GTK_CHECK_BUTTON(state->replace_spaces)));
  }
  return format;
}

SongList SongsFromState(const DialogState *state) {
  if (!state) {
    return {};
  }
  if (!state->songs.empty()) {
    return state->songs;
  }
  if (state->app && state->app->playlist_manager() && state->app->playlist_manager()->current()) {
    return state->app->playlist_manager()->current()->songs();
  }
  return {};
}

void ClearPreview(GtkWidget *list) {
  if (!list) {
    return;
  }
  GtkWidget *child = gtk_widget_get_first_child(list);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_list_box_remove(GTK_LIST_BOX(list), child);
    child = next;
  }
}

void RefreshPreview(DialogState *state) {
  if (!state) {
    return;
  }
  const OrganizeFormat format = FormatFromState(state);
  std::string error;
  const bool valid = OrganizeFormatValidator::IsValid(format.format(), &error);
  const std::string dest = state->dest ? gtk_editable_get_text(GTK_EDITABLE(state->dest)) : "";
  const SongList songs = SongsFromState(state);
  std::vector<OrganizePreview::Entry> entries =
      valid ? OrganizePreview::Compute(songs, format, state->transcode_mode, state->transcode_format, state->supported)
            : std::vector<OrganizePreview::Entry>{};
  if (OrganizePreview::AnyEmptyPath(entries)) {
    entries.clear();
  }
  ClearPreview(state->preview_list);
  if (state->preview_list) {
    for (const OrganizePreview::Entry &entry : entries) {
      GtkWidget *row = gtk_list_box_row_new();
      GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
      gtk_widget_set_margin_start(box, 8);
      gtk_widget_set_margin_end(box, 8);
      gtk_widget_set_margin_top(box, 4);
      gtk_widget_set_margin_bottom(box, 4);
      gtk_box_append(GTK_BOX(box), gtk_image_new_from_icon_name(OrganizePreview::PreviewIconName(entry)));
      GtkWidget *label = gtk_label_new(FileUtils::Join(dest, entry.relative_path).c_str());
      gtk_widget_set_halign(label, GTK_ALIGN_START);
      gtk_widget_set_hexpand(label, TRUE);
      gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
      gtk_box_append(GTK_BOX(box), label);
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
      gtk_list_box_append(GTK_LIST_BOX(state->preview_list), row);
    }
  }
  if (state->space) {
    state->space->SetPath(dest);
    if (state->space->total() > 0) {
      state->space->SetBytes(state->space->used(), OrganizePreview::TotalBytes(songs), state->space->total());
    }
  }
  if (state->run) {
    const int64_t used = state->space ? state->space->used() : 0;
    const int64_t total = state->space ? state->space->total() : 0;
    gtk_widget_set_sensitive(state->run, OrganizePreview::CanRun(valid, dest, entries, OrganizePreview::TotalBytes(songs), used, total));
  }
}

void PersistFromState(const DialogState *state) {
  if (!state || !state->buffer) {
    return;
  }
  GtkTextIter start;
  GtkTextIter end;
  gtk_text_buffer_get_bounds(state->buffer, &start, &end);
  gchar *text = gtk_text_buffer_get_text(state->buffer, &start, &end, FALSE);
  const std::string format_text = text ? text : "";
  g_free(text);
  Settings persist;
  persist.BeginGroup(OrganizeSettings::kSettingsGroup);
  persist.SetValue(OrganizeSettings::kFormat, format_text);
  if (state->persist_dest && state->dest) {
    persist.SetValue(OrganizeSettings::kDestination, gtk_editable_get_text(GTK_EDITABLE(state->dest)));
  }
  if (state->after_copy) {
    persist.SetBoolValue(OrganizeSettings::kMove, gtk_drop_down_get_selected(GTK_DROP_DOWN(state->after_copy)) == 1);
  }
  if (state->overwrite) {
    persist.SetBoolValue(OrganizeSettings::kOverwrite, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->overwrite)));
  }
  if (state->replace_spaces) {
    persist.SetBoolValue(OrganizeSettings::kReplaceSpaces, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->replace_spaces)));
  }
  if (state->remove_problematic) {
    persist.SetBoolValue(OrganizeSettings::kRemoveProblematic, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_problematic)));
  }
  if (state->remove_non_fat) {
    persist.SetBoolValue(OrganizeSettings::kRemoveNonFat, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_non_fat)));
  }
  if (state->remove_non_ascii) {
    persist.SetBoolValue(OrganizeSettings::kRemoveNonAscii, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->remove_non_ascii)));
  }
  if (state->allow_ascii_ext) {
    persist.SetBoolValue(OrganizeSettings::kAllowAsciiExt, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->allow_ascii_ext)));
  }
  if (state->albumcover) {
    persist.SetBoolValue(OrganizeSettings::kAlbumCover, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->albumcover)));
  }
  if (state->eject) {
    persist.SetBoolValue(OrganizeSettings::kEjectAfter, gtk_check_button_get_active(GTK_CHECK_BUTTON(state->eject)));
  }
  persist.Sync();
}

void RestoreDefaults(DialogState *state) {
  if (!state || !state->buffer) {
    return;
  }
  gtk_text_buffer_set_text(state->buffer, OrganizeSettings::kDefaultFormat, -1);
  if (state->remove_problematic) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->remove_problematic), TRUE);
  }
  if (state->remove_non_fat) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->remove_non_fat), FALSE);
  }
  if (state->remove_non_ascii) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->remove_non_ascii), FALSE);
  }
  if (state->allow_ascii_ext) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->allow_ascii_ext), FALSE);
    gtk_widget_set_sensitive(state->allow_ascii_ext, FALSE);
  }
  if (state->replace_spaces) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->replace_spaces), TRUE);
  }
  if (state->overwrite) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->overwrite), FALSE);
  }
  if (state->albumcover) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->albumcover), TRUE);
  }
  if (state->eject) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(state->eject), FALSE);
  }
}

}  // namespace

void OrganizeDialog::Show(GtkWindow *parent, Application *app, const SongList &songs, bool move) {
  Request request;
  request.songs = songs;
  request.move = move;
  Show(parent, app, request);
}

void OrganizeDialog::Show(GtkWindow *parent, Application *app, const Request &request) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Organize files"));
  adw_dialog_set_content_width(dialog, 560);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  Settings settings;
  settings.BeginGroup(OrganizeSettings::kSettingsGroup);
  const std::string saved_format = settings.Value(OrganizeSettings::kFormat, OrganizeSettings::kDefaultFormat);
  const std::string music_dir = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)
                                                                              : g_get_home_dir();
  std::string saved_dest = request.destination.empty() ? settings.Value(OrganizeSettings::kDestination, music_dir)
                                                       : request.destination;
  const std::vector<CollectionDirectory> dirs = app && app->collection() && app->collection()->backend()
                                                   ? app->collection()->backend()->Directories()
                                                   : std::vector<CollectionDirectory>{};
  if (saved_dest.empty() && !dirs.empty()) {
    saved_dest = dirs.front().path;
  }

  GtkWidget *format_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(format_header), gtk_label_new(Translations::CStr("Filename format")));
  GtkWidget *insert = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(insert), Translations::CStr("Insert..."));
  gtk_widget_set_hexpand(insert, TRUE);
  gtk_widget_set_halign(insert, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(format_header), insert);

  GtkWidget *format_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(format_view), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_size_request(format_view, -1, 56);
  gtk_widget_set_tooltip_text(format_view, Translations::CStr(OrganizeTokenHelp::Tooltip()));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(format_view));
  OrganizeSyntaxHighlighter highlighter;
  highlighter.Apply(buffer, saved_format);

  GtkWidget *tag_box = gtk_flow_box_new();
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(tag_box), 3);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(tag_box), GTK_SELECTION_NONE);
  for (const auto &tag : OrganizeFormat::InsertTags()) {
    GtkWidget *btn = gtk_button_new_with_label(Translations::CStr(tag.first));
    g_object_set_data(G_OBJECT(btn), "buffer", buffer);
    g_object_set_data_full(G_OBJECT(btn), "token", g_strdup((std::string("%") + tag.second).c_str()), g_free);
    g_signal_connect(btn, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *buf = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(button), "buffer"));
                       const char *token = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "token"));
                       if (!buf || !token) {
                         return;
                       }
                       GtkTextIter iter;
                       gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
                       gtk_text_buffer_insert(buf, &iter, token, -1);
                     })),
                     nullptr);
    gtk_flow_box_append(GTK_FLOW_BOX(tag_box), btn);
  }
  GtkWidget *popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER(popover), tag_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(insert), popover);

  GtkWidget *format_error = gtk_label_new("");
  gtk_widget_add_css_class(format_error, "error");
  gtk_label_set_xalign(GTK_LABEL(format_error), 0.0f);
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), saved_dest.c_str());
  GtkWidget *dest_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *browse = gtk_button_new_with_label(Translations::CStr("Choose folder…"));
  gtk_widget_set_hexpand(dest, TRUE);
  gtk_box_append(GTK_BOX(dest_row), dest);
  gtk_box_append(GTK_BOX(dest_row), browse);
  g_object_set_data(G_OBJECT(browse), "dest", dest);
  g_object_set_data(G_OBJECT(browse), "parent", parent);
  g_signal_connect(browse, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                     GtkFileDialog *chooser = gtk_file_dialog_new();
                     gtk_file_dialog_set_title(chooser, Translations::CStr("Organize destination"));
                     gtk_file_dialog_select_folder(chooser, GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), nullptr,
                                                   +[](GObject *source, GAsyncResult *result, gpointer user) {
                                                     auto *entry = GTK_EDITABLE(user);
                                                     GError *error = nullptr;
                                                     GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
                                                     if (!folder) {
                                                       if (error) {
                                                         g_error_free(error);
                                                       }
                                                       return;
                                                     }
                                                     gchar *path = g_file_get_path(folder);
                                                     if (path) {
                                                       gtk_editable_set_text(entry, path);
                                                     }
                                                     g_free(path);
                                                     g_object_unref(folder);
                                                   },
                                                   g_object_get_data(G_OBJECT(button), "dest"));
                   })),
                   nullptr);

  GtkWidget *dest_drop = nullptr;
  if (!dirs.empty()) {
    std::vector<std::string> labels;
    auto *paths = new std::vector<std::string>();
    guint selected = 0;
    for (size_t i = 0; i < dirs.size(); ++i) {
      labels.push_back(dirs[i].path);
      paths->push_back(dirs[i].path);
      if (dirs[i].path == saved_dest) {
        selected = static_cast<guint>(i);
      }
    }
    labels.push_back(Translations::Tr("Other folder…"));
    paths->push_back({});
    std::vector<const char *> cstr;
    cstr.reserve(labels.size() + 1);
    for (const std::string &label : labels) {
      cstr.push_back(label.c_str());
    }
    cstr.push_back(nullptr);
    dest_drop = gtk_drop_down_new_from_strings(cstr.data());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dest_drop), selected);
    g_object_set_data_full(G_OBJECT(dest_drop), "dest-paths", paths, [](gpointer p) { delete static_cast<std::vector<std::string> *>(p); });
    g_object_set_data(G_OBJECT(dest_drop), "dest", dest);
    g_signal_connect(dest_drop, "notify::selected", G_CALLBACK((+[](GtkDropDown *drop, GParamSpec *, gpointer) {
                       auto *dest_paths = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(drop), "dest-paths"));
                       auto *entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(drop), "dest"));
                       const guint index = gtk_drop_down_get_selected(drop);
                       if (!dest_paths || !entry || index >= dest_paths->size() || (*dest_paths)[index].empty()) {
                         return;
                       }
                       gtk_editable_set_text(entry, (*dest_paths)[index].c_str());
                     })),
                     nullptr);
  }

  const char *after_labels[] = {Translations::CStr("Keep the original files"), Translations::CStr("Delete the original files"), nullptr};
  GtkWidget *after_copy = gtk_drop_down_new_from_strings(after_labels);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(after_copy),
                             request.move || settings.BoolValue(OrganizeSettings::kMove, OrganizeSettings::kDefaultMove) ? 1 : 0);
  GtkWidget *overwrite = gtk_check_button_new_with_label(Translations::CStr("Overwrite existing files"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(overwrite), settings.BoolValue(OrganizeSettings::kOverwrite, OrganizeSettings::kDefaultOverwrite));
  GtkWidget *remove_problematic = gtk_check_button_new_with_label(Translations::CStr("Remove problematic characters from filenames"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(remove_problematic),
                              settings.BoolValue(OrganizeSettings::kRemoveProblematic, OrganizeSettings::kDefaultRemoveProblematic));
  GtkWidget *remove_non_fat = gtk_check_button_new_with_label(Translations::CStr("Restrict to characters allowed on FAT filesystems"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(remove_non_fat),
                              settings.BoolValue(OrganizeSettings::kRemoveNonFat, OrganizeSettings::kDefaultRemoveNonFat));
  GtkWidget *remove_non_ascii = gtk_check_button_new_with_label(Translations::CStr("Restrict characters to ASCII"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(remove_non_ascii),
                              settings.BoolValue(OrganizeSettings::kRemoveNonAscii, OrganizeSettings::kDefaultRemoveNonAscii));
  GtkWidget *allow_ascii_ext = gtk_check_button_new_with_label(Translations::CStr("Allow extended ASCII characters"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(allow_ascii_ext),
                              settings.BoolValue(OrganizeSettings::kAllowAsciiExt, OrganizeSettings::kDefaultAllowAsciiExt));
  gtk_widget_set_sensitive(allow_ascii_ext, gtk_check_button_get_active(GTK_CHECK_BUTTON(remove_non_ascii)));
  g_object_set_data(G_OBJECT(remove_non_ascii), "allow-ascii-ext", allow_ascii_ext);
  g_signal_connect(remove_non_ascii, "toggled", G_CALLBACK((+[](GtkCheckButton *button, gpointer) {
                     gtk_widget_set_sensitive(GTK_WIDGET(g_object_get_data(G_OBJECT(button), "allow-ascii-ext")),
                                              gtk_check_button_get_active(button));
                   })),
                   nullptr);
  GtkWidget *replace_spaces = gtk_check_button_new_with_label(Translations::CStr("Replace spaces with underscores"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(replace_spaces),
                              settings.BoolValue(OrganizeSettings::kReplaceSpaces, OrganizeSettings::kDefaultReplaceSpaces));
  GtkWidget *albumcover = gtk_check_button_new_with_label(Translations::CStr("Copy album cover art"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(albumcover), settings.BoolValue(OrganizeSettings::kAlbumCover, OrganizeSettings::kDefaultAlbumCover));
  GtkWidget *eject = nullptr;
  if (request.show_eject) {
    eject = gtk_check_button_new_with_label(Translations::CStr("Eject device afterwards"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(eject), settings.BoolValue(OrganizeSettings::kEjectAfter, OrganizeSettings::kDefaultEjectAfter));
  }
  auto *owned_songs = new SongList(request.songs);
  GtkWidget *status = gtk_label_new(owned_songs->empty() ? Translations::CStr("Uses the current playlist as the source.")
                                                         : (std::to_string(owned_songs->size()) + " selected song(s).").c_str());
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *preview_scroll = gtk_scrolled_window_new();
  gtk_widget_set_size_request(preview_scroll, -1, 140);
  gtk_widget_set_vexpand(preview_scroll, TRUE);
  GtkWidget *preview_list = gtk_list_box_new();
  gtk_widget_add_css_class(preview_list, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(preview_scroll), preview_list);
  auto *space = new FreeSpaceBar();
  space->SetPath(saved_dest);
  GtkWidget *run = gtk_button_new_with_label(Translations::CStr("Organize"));
  gtk_widget_add_css_class(run, "suggested-action");
  GtkWidget *cancel = gtk_button_new_with_label(Translations::CStr("Cancel"));
  gtk_widget_set_sensitive(cancel, FALSE);
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save settings"));
  GtkWidget *restore = gtk_button_new_with_label(Translations::CStr("Restore defaults"));

  auto *state = new DialogState;
  state->app = app;
  state->songs = *owned_songs;
  state->buffer = buffer;
  state->dest = dest;
  state->remove_problematic = remove_problematic;
  state->remove_non_fat = remove_non_fat;
  state->remove_non_ascii = remove_non_ascii;
  state->allow_ascii_ext = allow_ascii_ext;
  state->replace_spaces = replace_spaces;
  state->preview_list = preview_list;
  state->run = run;
  state->cancel = cancel;
  state->status = status;
  state->after_copy = after_copy;
  state->overwrite = overwrite;
  state->albumcover = albumcover;
  state->eject = eject;
  state->format_error = format_error;
  state->space = space;
  state->transcode_mode = request.transcode_mode;
  state->transcode_format = request.transcode_format;
  state->supported = request.supported_filetypes;
  state->persist_dest = request.destination.empty();
  g_signal_connect(space->widget(), "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) { delete static_cast<FreeSpaceBar *>(data); }), space);
  g_object_set_data_full(G_OBJECT(run), "state", state, [](gpointer p) { delete static_cast<DialogState *>(p); });

  g_object_set_data(G_OBJECT(run), "buffer", buffer);
  g_object_set_data(G_OBJECT(run), "dest", dest);
  g_object_set_data(G_OBJECT(run), "after-copy", after_copy);
  g_object_set_data(G_OBJECT(run), "overwrite", overwrite);
  g_object_set_data(G_OBJECT(run), "remove-problematic", remove_problematic);
  g_object_set_data(G_OBJECT(run), "remove-non-fat", remove_non_fat);
  g_object_set_data(G_OBJECT(run), "remove-non-ascii", remove_non_ascii);
  g_object_set_data(G_OBJECT(run), "allow-ascii-ext", allow_ascii_ext);
  g_object_set_data(G_OBJECT(run), "replace-spaces", replace_spaces);
  g_object_set_data(G_OBJECT(run), "albumcover", albumcover);
  g_object_set_data(G_OBJECT(run), "status", status);
  g_object_set_data(G_OBJECT(run), "parent", parent);
  g_object_set_data(G_OBJECT(run), "transcode-mode", GINT_TO_POINTER(static_cast<int>(request.transcode_mode)));
  g_object_set_data(G_OBJECT(run), "transcode-format", GINT_TO_POINTER(static_cast<int>(request.transcode_format)));
  g_object_set_data(G_OBJECT(run), "persist-dest", GINT_TO_POINTER(request.destination.empty() ? 1 : 2));
  auto *supported = new std::vector<Song::FileType>(request.supported_filetypes);
  g_object_set_data_full(G_OBJECT(run), "supported", supported, [](gpointer p) { delete static_cast<std::vector<Song::FileType> *>(p); });
  if (eject) {
    g_object_set_data(G_OBJECT(run), "eject", eject);
  }
  if (!request.device_id.empty()) {
    g_object_set_data_full(G_OBJECT(run), "device-id", g_strdup(request.device_id.c_str()), g_free);
  }
  g_object_set_data_full(G_OBJECT(run), "songs", owned_songs, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_object_set_data(G_OBJECT(buffer), "error", format_error);
  g_object_set_data(G_OBJECT(buffer), "state", state);

  g_signal_connect(buffer, "changed", G_CALLBACK(+[](GtkTextBuffer *buf, gpointer data) {
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buf, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
                     const std::string format = text ? text : "";
                     g_free(text);
                     OrganizeSyntaxHighlighter highlight;
                     highlight.Highlight(buf, format);
                     std::string error;
                     const bool valid = OrganizeFormatValidator::IsValid(format, &error);
                     gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(buf), "error")), valid ? "" : error.c_str());
                     RefreshPreview(static_cast<DialogState *>(data));
                   }),
                   state);
  g_signal_connect(dest, "changed", G_CALLBACK(+[](GtkEditable *, gpointer data) { RefreshPreview(static_cast<DialogState *>(data)); }), state);
  for (GtkWidget *toggle : {remove_problematic, remove_non_fat, remove_non_ascii, allow_ascii_ext, replace_spaces}) {
    g_signal_connect(toggle, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) { RefreshPreview(static_cast<DialogState *>(data)); }),
                     state);
  }
  g_signal_connect(run, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *buf = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(button), "buffer"));
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buf, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
                     const std::string format_text = text ? text : "";
                     g_free(text);
                     std::string error;
                     if (!OrganizeFormatValidator::IsValid(format_text, &error)) {
                       gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "status")), error.c_str());
                       return;
                     }
                     OrganizeFormat format(format_text);
                     format.set_remove_problematic(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "remove-problematic"))));
                     format.set_remove_non_fat(gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "remove-non-fat"))));
                     format.set_remove_non_ascii(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "remove-non-ascii"))));
                     format.set_allow_ascii_ext(gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "allow-ascii-ext"))));
                     format.set_replace_spaces(gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "replace-spaces"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     Organize::Options options;
                     options.move = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "after-copy"))) == 1;
                     options.overwrite = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "overwrite")));
                     options.albumcover = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "albumcover")));
                     options.transcode_mode = static_cast<MusicStorage::TranscodeMode>(
                         GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "transcode-mode")));
                     options.transcode_format = static_cast<Song::FileType>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "transcode-format")));
                     if (auto *supported = static_cast<std::vector<Song::FileType> *>(g_object_get_data(G_OBJECT(button), "supported"))) {
                       options.supported_filetypes = *supported;
                     }
                     if (application && application->collection()) {
                       options.collection_backend = application->collection()->backend();
                       options.destination_is_collection =
                           OrganizePathNotify::DestinationIsCollection(dest_dir, options.collection_backend);
                       options.collection_directory_id = OrganizePathNotify::DirectoryIdForPath(dest_dir, options.collection_backend);
                     }
                     if (application) {
                       options.tagreader = application->tagreader();
                     }
                     options.cover_cache_path = FileUtils::Join(StandardPaths::CacheDir(), "organize-cover.bin");
                     auto *state = static_cast<DialogState *>(g_object_get_data(G_OBJECT(button), "state"));
                     PersistFromState(state);
                     if (state && state->job) {
                       return;
                     }
                     auto *owned = static_cast<SongList *>(g_object_get_data(G_OBJECT(button), "songs"));
                     SongList songs = owned && !owned->empty() ? *owned
                                      : application->playlist_manager()->current() ? application->playlist_manager()->current()->songs()
                                                                                   : SongList{};
                     auto *job = new Organize(application ? application->task_manager() : nullptr);
                     if (state) {
                       state->job = job;
                     }
                     gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
                     if (state && state->cancel) {
                       gtk_widget_set_sensitive(state->cancel, TRUE);
                     }
                     GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                     gtk_label_set_text(GTK_LABEL(status_label), OrganizeJob::TaskName());
                     const std::shared_ptr<bool> alive = state ? state->alive : std::make_shared<bool>(true);
                     job->Finished.Connect([state, alive, job, application, button](Organize *) {
                       if (alive && *alive && state && state->job == job) {
                         state->job = nullptr;
                         gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
                         if (state->cancel) {
                           gtk_widget_set_sensitive(state->cancel, FALSE);
                         }
                         GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                         const std::vector<Organize::Error> &errors = job->errors();
                         if (errors.empty()) {
                           gtk_label_set_text(GTK_LABEL(status_label), Translations::CStr("Organize finished."));
                           gpointer eject_ptr = g_object_get_data(G_OBJECT(button), "eject");
                           const char *device_id = static_cast<const char *>(g_object_get_data(G_OBJECT(button), "device-id"));
                           if (eject_ptr && device_id && gtk_check_button_get_active(GTK_CHECK_BUTTON(eject_ptr)) && application &&
                               application->device_manager()) {
                             application->device_manager()->Unmount(device_id);
                           }
                         } else {
                           OrganizeErrorDialog::Show(GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), errors);
                           gtk_label_set_text(GTK_LABEL(status_label), (std::to_string(errors.size()) + " file(s) failed.").c_str());
                         }
                       }
                       if (application && application->collection()) {
                         application->collection()->IncrementalScan();
                       }
                       g_idle_add(+[](gpointer data) -> gboolean {
                         delete static_cast<Organize *>(data);
                         return G_SOURCE_REMOVE;
                       }, job);
                     });
                     job->Start(songs, dest_dir, format, options);
                   })),
                   app);

  gtk_box_append(GTK_BOX(box), format_header);
  gtk_box_append(GTK_BOX(box), format_view);
  gtk_box_append(GTK_BOX(box), format_error);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Destination")));
  if (dest_drop) {
    gtk_box_append(GTK_BOX(box), dest_drop);
  }
  gtk_box_append(GTK_BOX(box), dest_row);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("After copying")));
  gtk_box_append(GTK_BOX(box), after_copy);
  gtk_box_append(GTK_BOX(box), space->widget());
  gtk_box_append(GTK_BOX(box), overwrite);
  gtk_box_append(GTK_BOX(box), remove_problematic);
  gtk_box_append(GTK_BOX(box), remove_non_fat);
  gtk_box_append(GTK_BOX(box), remove_non_ascii);
  gtk_box_append(GTK_BOX(box), allow_ascii_ext);
  gtk_box_append(GTK_BOX(box), replace_spaces);
  gtk_box_append(GTK_BOX(box), albumcover);
  if (eject) {
    gtk_box_append(GTK_BOX(box), eject);
  }
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Preview")));
  gtk_box_append(GTK_BOX(box), preview_scroll);
  GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(actions), restore);
  gtk_box_append(GTK_BOX(actions), save);
  gtk_widget_set_hexpand(save, TRUE);
  gtk_widget_set_halign(save, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(actions), cancel);
  gtk_box_append(GTK_BOX(actions), run);
  g_signal_connect(cancel, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *state = static_cast<DialogState *>(data);
                     if (state && state->job) {
                       state->job->Cancel();
                       if (state->status) {
                         gtk_label_set_text(GTK_LABEL(state->status), Translations::CStr("Cancelling..."));
                       }
                     }
                   }),
                   state);
  gtk_box_append(GTK_BOX(box), actions);
  gtk_box_append(GTK_BOX(box), status);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *s = static_cast<DialogState *>(data);
                     PersistFromState(s);
                     if (s && s->status) {
                       gtk_label_set_text(GTK_LABEL(s->status), Translations::CStr("Settings saved."));
                     }
                   }),
                   state);
  g_signal_connect(restore, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     RestoreDefaults(static_cast<DialogState *>(data));
                   }),
                   state);
  adw_dialog_set_child(dialog, box);
  std::string format_error_text;
  gtk_label_set_text(GTK_LABEL(format_error), OrganizeFormatValidator::IsValid(saved_format, &format_error_text) ? "" : format_error_text.c_str());
  RefreshPreview(state);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
