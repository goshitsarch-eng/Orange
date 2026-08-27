#include "widgets/playingwidget.h"

#include "constants/playingwidgetsettings.h"
#include "context/contextcover.h"
#include "core/settings.h"
#include "covermanager/coverchoicemenu.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

PlayingWidget::PlayingWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(widget_, "playing-widget");
  previous_cover_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(previous_cover_), kSmallCover);
  gtk_widget_set_visible(previous_cover_, FALSE);
  cover_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), kSmallCover);
  GtkWidget *overlay = gtk_overlay_new();
  gtk_overlay_set_child(GTK_OVERLAY(overlay), previous_cover_);
  gtk_overlay_add_overlay(GTK_OVERLAY(overlay), cover_);
  spinner_ = gtk_spinner_new();
  gtk_widget_set_visible(spinner_, FALSE);
  labels_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  title_ = gtk_label_new(Translations::CStr("Not playing"));
  gtk_widget_add_css_class(title_, "heading");
  gtk_label_set_wrap(GTK_LABEL(title_), TRUE);
  gtk_widget_set_halign(title_, GTK_ALIGN_START);
  artist_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(artist_), TRUE);
  gtk_widget_set_halign(artist_, GTK_ALIGN_START);
  album_ = gtk_label_new("");
  gtk_widget_add_css_class(album_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(album_), TRUE);
  gtk_widget_set_halign(album_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(labels_), title_);
  gtk_box_append(GTK_BOX(labels_), artist_);
  gtk_box_append(GTK_BOX(labels_), album_);
  gtk_box_append(GTK_BOX(widget_), overlay);
  gtk_box_append(GTK_BOX(widget_), spinner_);
  gtk_box_append(GTK_BOX(widget_), labels_);

  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK((+[](GtkGestureClick *gesture, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlayingWidget *>(data);
                     self->ShowMenu(x, y);
                     gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
                   })),
                   this);

  GtkDropTarget *target = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_COPY);
#ifdef GDK_TYPE_FILE_LIST
  GType types[] = {G_TYPE_STRING, GDK_TYPE_FILE_LIST};
  gtk_drop_target_set_gtypes(target, types, 2);
#endif
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(target));
  g_signal_connect(target, "drop", G_CALLBACK((+[](GtkDropTarget *, const GValue *value, gdouble, gdouble, gpointer data) -> gboolean {
                     return static_cast<PlayingWidget *>(data)->OnDrop(value);
                   })),
                   this);

  gtk_widget_set_overflow(widget_, GTK_OVERFLOW_HIDDEN);
  LoadSettings();
  ApplyLayout();
  ApplyVisibility(false);
}

PlayingWidget::~PlayingWidget() {
  StopFade();
  StopShowHide();
}

void PlayingWidget::LoadSettings() {
  Settings settings;
  settings.BeginGroup(PlayingWidgetSettings::kSettingsGroup);
  mode_ = static_cast<Mode>(settings.IntValue(PlayingWidgetSettings::kMode, PlayingWidgetSettings::kDefaultMode));
  above_status_bar_ = settings.BoolValue(PlayingWidgetSettings::kAboveStatusBar, PlayingWidgetSettings::kDefaultAboveStatusBar);
  fit_cover_width_ = settings.BoolValue(PlayingWidgetSettings::kFitCoverWidth, PlayingWidgetSettings::kDefaultFitCoverWidth);
}

void PlayingWidget::SaveSettings() const {
  Settings settings;
  settings.BeginGroup(PlayingWidgetSettings::kSettingsGroup);
  settings.SetIntValue(PlayingWidgetSettings::kMode, static_cast<int>(mode_));
  settings.SetBoolValue(PlayingWidgetSettings::kAboveStatusBar, above_status_bar_);
  settings.SetBoolValue(PlayingWidgetSettings::kFitCoverWidth, fit_cover_width_);
  settings.Sync();
}

void PlayingWidget::SetEnabled(bool enabled) {
  enabled_ = enabled;
  ApplyVisibility();
}

void PlayingWidget::SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }

void PlayingWidget::SetCoverActionCallback(CoverActionCallback callback) { cover_action_ = std::move(callback); }

void PlayingWidget::SetMode(Mode mode) {
  mode_ = mode;
  SaveSettings();
  ApplyLayout();
}

void PlayingWidget::SetAboveStatusBar(bool above) {
  if (above_status_bar_ == above) {
    return;
  }
  above_status_bar_ = above;
  SaveSettings();
  AboveStatusBarChanged.Emit(above);
}

void PlayingWidget::SetFitCoverWidth(bool fit) {
  fit_cover_width_ = fit;
  SaveSettings();
  ApplyLayout();
}

void PlayingWidget::ApplyLayout() {
  const bool large = mode_ == Mode::LargeSongDetails;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(widget_), large ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
  const int size = CoverSize(mode_, fit_cover_width_, gtk_widget_get_width(widget_));
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), size);
  gtk_image_set_pixel_size(GTK_IMAGE(previous_cover_), size);
  gtk_widget_set_size_request(cover_, size, size);
  gtk_widget_set_size_request(previous_cover_, size, size);
  gtk_widget_set_hexpand(cover_, large && fit_cover_width_);
  gtk_widget_set_halign(cover_, large ? GTK_ALIGN_CENTER : GTK_ALIGN_START);
  gtk_widget_set_halign(previous_cover_, large ? GTK_ALIGN_CENTER : GTK_ALIGN_START);
}

int PlayingWidget::CurrentTotalHeight() const {
  const int cover = CoverSize(mode_, fit_cover_width_, gtk_widget_get_width(widget_));
  int details = labels_ ? gtk_widget_get_height(labels_) : 0;
  if (details <= 0) {
    details = DetailsEstimate(!DetailsAlbum(song_).empty());
  }
  return TotalHeight(mode_, cover, details);
}

void PlayingWidget::ApplyShowHideHeight() {
  const int height = AnimatedHeight(CurrentTotalHeight(), showhide_elapsed_ms_);
  gtk_widget_set_size_request(widget_, -1, height);
  gtk_widget_set_visible(widget_, height > 0 || showhide_target_);
}

void PlayingWidget::StopShowHide() {
  if (showhide_timeout_id_) {
    g_source_remove(showhide_timeout_id_);
    showhide_timeout_id_ = 0;
  }
}

void PlayingWidget::StartShowHide(bool show) {
  showhide_target_ = show;
  if (showhide_timeout_id_) {
    return;
  }
  if (show) {
    gtk_widget_set_visible(widget_, TRUE);
  }
  showhide_timeout_id_ =
      g_timeout_add(kFadeTickMs, [](gpointer data) -> gboolean { return static_cast<PlayingWidget *>(data)->ShowHideTick(); }, this);
}

gboolean PlayingWidget::ShowHideTick() {
  showhide_elapsed_ms_ = ShowHideElapsed(showhide_target_, showhide_elapsed_ms_, kFadeTickMs);
  ApplyShowHideHeight();
  if (!ShowHideFinished(showhide_target_, showhide_elapsed_ms_)) {
    return G_SOURCE_CONTINUE;
  }
  showhide_timeout_id_ = 0;
  shown_ = showhide_target_;
  if (shown_) {
    gtk_widget_set_size_request(widget_, -1, -1);
    gtk_widget_set_visible(widget_, TRUE);
  } else {
    gtk_widget_set_visible(widget_, FALSE);
  }
  return G_SOURCE_REMOVE;
}

void PlayingWidget::ApplyVisibility(bool animate) {
  const bool want = ShouldShow(enabled_, active_);
  if (!animate) {
    StopShowHide();
    shown_ = want;
    showhide_target_ = want;
    showhide_elapsed_ms_ = want ? kShowHideMs : 0;
    gtk_widget_set_visible(widget_, want);
    gtk_widget_set_size_request(widget_, -1, want ? -1 : 0);
    return;
  }
  if (want == shown_ && !showhide_timeout_id_) {
    return;
  }
  StartShowHide(want);
}

void PlayingWidget::Playing() { playing_ = true; }

void PlayingWidget::Stopped() {
  playing_ = false;
  active_ = false;
  StopFade();
  gtk_widget_set_opacity(cover_, 1.0);
  gtk_widget_set_visible(previous_cover_, FALSE);
  SongChanged(Song());
  SetImageFromBytes(cover_, {});
  ApplyVisibility();
}

void PlayingWidget::Error() {
  playing_ = false;
  gtk_label_set_text(GTK_LABEL(title_), Translations::CStr("Error"));
}

void PlayingWidget::SongChanged(const Song &song) {
  song_ = song;
  const std::string title = DetailsTitle(song);
  gtk_label_set_text(GTK_LABEL(title_), title.empty() ? Translations::CStr("Not playing") : title.c_str());
  gtk_label_set_text(GTK_LABEL(artist_), DetailsArtist(song).c_str());
  gtk_label_set_text(GTK_LABEL(album_), DetailsAlbum(song).c_str());
  gtk_widget_set_visible(album_, !DetailsAlbum(song).empty());
  if (playing_) {
    active_ = true;
    ApplyVisibility();
  }
}

void PlayingWidget::SetCover(const std::vector<unsigned char> &data) {
  gtk_widget_set_visible(spinner_, FALSE);
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  SnapshotCurrentToPrevious();
  SetImageFromBytes(cover_, data);
  if (playing_) {
    active_ = true;
    ApplyVisibility();
  }
  StartFade();
}

void PlayingWidget::SearchCoverInProgress() {
  gtk_widget_set_visible(spinner_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
}

void PlayingWidget::SnapshotCurrentToPrevious() {
  GdkPaintable *paintable = gtk_image_get_paintable(GTK_IMAGE(cover_));
  if (paintable) {
    gtk_image_set_from_paintable(GTK_IMAGE(previous_cover_), paintable);
  } else {
    gtk_image_set_from_icon_name(GTK_IMAGE(previous_cover_), "audio-x-generic-symbolic");
  }
  gtk_image_set_pixel_size(GTK_IMAGE(previous_cover_), gtk_image_get_pixel_size(GTK_IMAGE(cover_)));
  gtk_widget_set_visible(previous_cover_, TRUE);
  gtk_widget_set_opacity(previous_cover_, 1.0);
}

void PlayingWidget::SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data) {
  const int size = CoverSize(mode_, fit_cover_width_, gtk_widget_get_width(widget_));
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), size);
    return;
  }
  GBytes *bytes = g_bytes_new(data.data(), data.size());
  GError *error = nullptr;
  GdkTexture *texture = gdk_texture_new_from_bytes(bytes, &error);
  g_bytes_unref(bytes);
  if (!texture) {
    if (error) {
      g_error_free(error);
    }
    return;
  }
  gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(image), size);
  g_object_unref(texture);
}

void PlayingWidget::StartFade() {
  StopFade();
  fade_elapsed_ms_ = 0;
  gtk_widget_set_opacity(cover_, 0.0);
  fade_timeout_id_ = g_timeout_add(kFadeTickMs, [](gpointer data) -> gboolean { return static_cast<PlayingWidget *>(data)->FadeTick(); }, this);
}

void PlayingWidget::StopFade() {
  if (fade_timeout_id_) {
    g_source_remove(fade_timeout_id_);
    fade_timeout_id_ = 0;
  }
  fade_elapsed_ms_ = 0;
  gtk_widget_set_opacity(cover_, 1.0);
}

gboolean PlayingWidget::FadeTick() {
  fade_elapsed_ms_ += kFadeTickMs;
  gtk_widget_set_opacity(previous_cover_, FadeOutOpacity(fade_elapsed_ms_));
  gtk_widget_set_opacity(cover_, FadeInOpacity(fade_elapsed_ms_));
  if (fade_elapsed_ms_ < kFadeTimelineMs) {
    return G_SOURCE_CONTINUE;
  }
  fade_timeout_id_ = 0;
  gtk_widget_set_opacity(cover_, 1.0);
  gtk_widget_set_visible(previous_cover_, FALSE);
  gtk_image_set_from_icon_name(GTK_IMAGE(previous_cover_), "audio-x-generic-symbolic");
  return G_SOURCE_REMOVE;
}

gboolean PlayingWidget::OnDrop(const GValue *value) {
  std::vector<std::string> paths;
  if (G_VALUE_HOLDS_STRING(value)) {
    const char *text = g_value_get_string(value);
    for (const std::string &part : StrUtils::Split(text ? text : "", '\n')) {
      std::string url = part;
      if (!url.empty() && url.back() == '\r') {
        url.pop_back();
      }
      if (!url.empty()) {
        paths.push_back(url);
      }
    }
  }
#ifdef GDK_TYPE_FILE_LIST
  if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
    auto *list = static_cast<GdkFileList *>(g_value_get_boxed(value));
    GSList *files = gdk_file_list_get_files(list);
    for (GSList *item = files; item; item = item->next) {
      gchar *uri = g_file_get_uri(G_FILE(item->data));
      if (uri) {
        paths.emplace_back(uri);
        g_free(uri);
      }
    }
  }
#endif
  for (const std::string &url : paths) {
    const std::string path = FileUtils::PathFromUri(url);
    if (!IsImagePath(path) && !IsImagePath(url)) {
      continue;
    }
    const std::string data = FileUtils::ReadFile(path);
    if (data.empty() || !JsonUtils::LooksLikeImage(data)) {
      continue;
    }
    if (drop_) {
      drop_(std::vector<unsigned char>(data.begin(), data.end()));
    }
    return TRUE;
  }
  return FALSE;
}

void PlayingWidget::ShowMenu(double x, double y) {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::CStr("Small album cover"), "playing.small");
  g_menu_append(menu, Translations::CStr("Large album cover"), "playing.large");
  g_menu_append(menu, Translations::CStr("Show above status bar"), "playing.above");
  g_menu_append(menu, Translations::CStr("Fit cover width"), "playing.fit");
  const bool show_cover = CoverChoiceMenu::HasCoverActions(static_cast<bool>(cover_action_), song_.is_valid());
  if (show_cover) {
    GMenu *cover = g_menu_new();
    for (const CoverChoiceMenu::Item &item : CoverChoiceMenu::Items()) {
      g_menu_append(cover, Translations::CStr(item.label), CoverChoiceMenu::ActionPath("cover", item.id).c_str());
    }
    g_menu_append(cover, Translations::CStr(CoverChoiceMenu::SearchAutomaticallyLabel()),
                  CoverChoiceMenu::SearchAutomaticallyPath("cover").c_str());
    g_menu_append_section(menu, Translations::CStr("Cover"), G_MENU_MODEL(cover));
    g_object_unref(cover);
  }
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_widget_set_parent(popover, widget_);
  GdkRectangle rect{static_cast<int>(x), static_cast<int>(y), 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
  GSimpleActionGroup *group = g_simple_action_group_new();
  auto add = [&](const char *name, GCallback callback) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_signal_connect(action, "activate", callback, this);
    g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(action));
    g_object_unref(action);
  };
  add("small", G_CALLBACK((+[](GSimpleAction *, GVariant *, gpointer data) {
        static_cast<PlayingWidget *>(data)->SetMode(Mode::SmallSongDetails);
      })));
  add("large", G_CALLBACK((+[](GSimpleAction *, GVariant *, gpointer data) {
        static_cast<PlayingWidget *>(data)->SetMode(Mode::LargeSongDetails);
      })));
  add("above", G_CALLBACK((+[](GSimpleAction *, GVariant *, gpointer data) {
        auto *self = static_cast<PlayingWidget *>(data);
        self->SetAboveStatusBar(!self->above_status_bar());
      })));
  GSimpleAction *fit = g_simple_action_new("fit", nullptr);
  g_simple_action_set_enabled(fit, FitCoverWidthEnabled(mode_) ? TRUE : FALSE);
  g_signal_connect(fit, "activate", G_CALLBACK((+[](GSimpleAction *, GVariant *, gpointer data) {
                     auto *self = static_cast<PlayingWidget *>(data);
                     if (FitCoverWidthEnabled(self->mode())) {
                       self->SetFitCoverWidth(!self->fit_cover_width());
                     }
                   })),
                   this);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(fit));
  g_object_unref(fit);
  gtk_widget_insert_action_group(popover, "playing", G_ACTION_GROUP(group));
  if (show_cover) {
    GSimpleActionGroup *cover_group = g_simple_action_group_new();
    for (const CoverChoiceMenu::Item &item : CoverChoiceMenu::Items()) {
      GSimpleAction *action = g_simple_action_new(item.id, nullptr);
      g_signal_connect(action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                         auto *self = static_cast<PlayingWidget *>(data);
                         if (self->cover_action_) {
                           self->cover_action_(CoverChoiceMenu::FromId(g_action_get_name(G_ACTION(act))));
                         }
                       }),
                       this);
      g_action_map_add_action(G_ACTION_MAP(cover_group), G_ACTION(action));
      g_object_unref(action);
    }
    Settings auto_settings;
    GSimpleAction *auto_action = g_simple_action_new_stateful(
        CoverChoiceMenu::SearchAutomaticallyId(), nullptr, g_variant_new_boolean(ContextCover::LoadEnabled(auto_settings)));
    g_signal_connect(auto_action, "activate", G_CALLBACK(+[](GSimpleAction *act, GVariant *, gpointer data) {
                       auto *self = static_cast<PlayingWidget *>(data);
                       Settings settings;
                       const bool enabled = ContextCover::ToggleEnabled(settings);
                       g_simple_action_set_state(act, g_variant_new_boolean(enabled));
                       if (self->search_auto_changed_) {
                         self->search_auto_changed_(enabled);
                       }
                     }),
                     this);
    g_action_map_add_action(G_ACTION_MAP(cover_group), G_ACTION(auto_action));
    g_object_unref(auto_action);
    gtk_widget_insert_action_group(popover, "cover", G_ACTION_GROUP(cover_group));
    g_object_unref(cover_group);
  }
  g_object_unref(group);
  g_object_unref(menu);
  gtk_popover_popup(GTK_POPOVER(popover));
}
