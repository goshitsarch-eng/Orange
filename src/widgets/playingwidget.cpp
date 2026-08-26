#include "widgets/playingwidget.h"

#include "constants/playingwidgetsettings.h"
#include "core/settings.h"
#include "translations/translations.h"

PlayingWidget::PlayingWidget() {
  widget_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(widget_, "playing-widget");
  cover_ = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), kSmallCover);
  spinner_ = gtk_spinner_new();
  gtk_widget_set_visible(spinner_, FALSE);
  GtkWidget *labels = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  title_ = gtk_label_new(Translations::CStr("Not playing"));
  gtk_widget_add_css_class(title_, "heading");
  gtk_label_set_wrap(GTK_LABEL(title_), TRUE);
  gtk_widget_set_halign(title_, GTK_ALIGN_START);
  artist_ = gtk_label_new("");
  gtk_widget_add_css_class(artist_, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(artist_), TRUE);
  gtk_widget_set_halign(artist_, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(labels), title_);
  gtk_box_append(GTK_BOX(labels), artist_);
  gtk_box_append(GTK_BOX(widget_), cover_);
  gtk_box_append(GTK_BOX(widget_), spinner_);
  gtk_box_append(GTK_BOX(widget_), labels);

  GtkGesture *menu = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(menu), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller(widget_, GTK_EVENT_CONTROLLER(menu));
  g_signal_connect(menu, "pressed", G_CALLBACK((+[](GtkGestureClick *gesture, gint, gdouble x, gdouble y, gpointer data) {
                     auto *self = static_cast<PlayingWidget *>(data);
                     self->ShowMenu(x, y);
                     gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
                   })),
                   this);

  LoadSettings();
  ApplyLayout();
}

PlayingWidget::~PlayingWidget() { StopFade(); }

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
  gtk_widget_set_visible(widget_, enabled);
}

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
  gtk_widget_set_size_request(cover_, size, size);
  gtk_widget_set_hexpand(cover_, large && fit_cover_width_);
  gtk_widget_set_halign(cover_, large ? GTK_ALIGN_CENTER : GTK_ALIGN_START);
}

void PlayingWidget::Playing() { playing_ = true; }

void PlayingWidget::Stopped() {
  playing_ = false;
  SongChanged(Song());
  SetCover({});
}

void PlayingWidget::Error() {
  playing_ = false;
  gtk_label_set_text(GTK_LABEL(title_), Translations::CStr("Error"));
}

void PlayingWidget::SongChanged(const Song &song) {
  song_ = song;
  gtk_label_set_text(GTK_LABEL(title_), song.PrettyTitle().empty() ? Translations::CStr("Not playing") : song.PrettyTitle().c_str());
  gtk_label_set_text(GTK_LABEL(artist_), song.EffectiveAlbumartist().c_str());
}

void PlayingWidget::SetCover(const std::vector<unsigned char> &data) {
  gtk_widget_set_visible(spinner_, FALSE);
  gtk_spinner_stop(GTK_SPINNER(spinner_));
  SetImageFromBytes(data);
  StartFade();
}

void PlayingWidget::SearchCoverInProgress() {
  gtk_widget_set_visible(spinner_, TRUE);
  gtk_spinner_start(GTK_SPINNER(spinner_));
}

void PlayingWidget::SetImageFromBytes(const std::vector<unsigned char> &data) {
  const int size = CoverSize(mode_, fit_cover_width_, gtk_widget_get_width(widget_));
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(cover_), "audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(cover_), size);
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
  gtk_image_set_from_paintable(GTK_IMAGE(cover_), GDK_PAINTABLE(texture));
  gtk_image_set_pixel_size(GTK_IMAGE(cover_), size);
  g_object_unref(texture);
}

void PlayingWidget::StartFade() {
  StopFade();
  fade_elapsed_ms_ = 0;
  gtk_widget_set_opacity(cover_, 0.0);
  fade_timeout_id_ = g_timeout_add(50, [](gpointer data) -> gboolean {
    auto *self = static_cast<PlayingWidget *>(data);
    self->fade_elapsed_ms_ += 50;
    gtk_widget_set_opacity(self->cover_, FadeInOpacity(self->fade_elapsed_ms_));
    if (self->fade_elapsed_ms_ >= kFadeTimelineMs) {
      self->fade_timeout_id_ = 0;
      return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
  }, this);
}

void PlayingWidget::StopFade() {
  if (fade_timeout_id_) {
    g_source_remove(fade_timeout_id_);
    fade_timeout_id_ = 0;
  }
  gtk_widget_set_opacity(cover_, 1.0);
}

void PlayingWidget::ShowMenu(double x, double y) {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, Translations::CStr("Small album cover"), "playing.small");
  g_menu_append(menu, Translations::CStr("Large album cover"), "playing.large");
  g_menu_append(menu, Translations::CStr("Show above status bar"), "playing.above");
  g_menu_append(menu, Translations::CStr("Fit cover width"), "playing.fit");
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
  add("fit", G_CALLBACK((+[](GSimpleAction *, GVariant *, gpointer data) {
        auto *self = static_cast<PlayingWidget *>(data);
        if (self->mode() == Mode::LargeSongDetails) {
          self->SetFitCoverWidth(!self->fit_cover_width());
        }
      })));
  gtk_widget_insert_action_group(popover, "playing", G_ACTION_GROUP(group));
  g_object_unref(group);
  g_object_unref(menu);
  gtk_popover_popup(GTK_POPOVER(popover));
}
