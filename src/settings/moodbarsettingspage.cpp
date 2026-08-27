#include "settings/moodbarsettingspage.h"

#include "constants/moodbarsettings.h"
#include "core/seekbarsettings.h"
#include "moodbar/moodbarpreview.h"
#include "moodbar/moodbarstyle.h"
#include "settings/moodbarsettingslabels.h"
#include "settings/settingspage.h"
#include "widgets/seekbarmode.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <cstring>
#include <vector>

namespace {

struct MoodbarPreviewPixbufs {
  std::vector<GdkPixbuf *> pixbufs;
};

void FreePreviewPixbufs(gpointer data) {
  auto *previews = static_cast<MoodbarPreviewPixbufs *>(data);
  if (!previews) {
    return;
  }
  for (GdkPixbuf *pixbuf : previews->pixbufs) {
    if (pixbuf) {
      g_object_unref(pixbuf);
    }
  }
  delete previews;
}

GdkPixbuf *PixbufFromStripe(const std::vector<uint8_t> &stripe) {
  GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, MoodbarPreview::kWidth, MoodbarPreview::kHeight);
  if (!pixbuf || stripe.size() < static_cast<size_t>(MoodbarPreview::kWidth) * 3) {
    return pixbuf;
  }
  guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
  const int stride = gdk_pixbuf_get_rowstride(pixbuf);
  for (int y = 0; y < MoodbarPreview::kHeight; ++y) {
    std::memcpy(pixels + y * stride, stripe.data(), static_cast<size_t>(MoodbarPreview::kWidth) * 3);
  }
  return pixbuf;
}

void PreviewSetup(GtkListItemFactory *, GtkListItem *item) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *picture = gtk_picture_new();
  gtk_widget_set_size_request(picture, MoodbarPreview::kWidth, MoodbarPreview::kHeight);
  gtk_picture_set_can_shrink(GTK_PICTURE(picture), FALSE);
  gtk_widget_set_valign(picture, GTK_ALIGN_CENTER);
  GtkWidget *label = gtk_label_new("");
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(box), picture);
  gtk_box_append(GTK_BOX(box), label);
  gtk_list_item_set_child(item, box);
}

void PreviewBind(GtkListItemFactory *factory, GtkListItem *item) {
  auto *previews = static_cast<MoodbarPreviewPixbufs *>(g_object_get_data(G_OBJECT(factory), "pixbufs"));
  GtkWidget *box = gtk_list_item_get_child(item);
  if (!previews || !box) {
    return;
  }
  const guint index = gtk_list_item_get_position(item);
  GtkWidget *picture = gtk_widget_get_first_child(box);
  GtkWidget *label = picture ? gtk_widget_get_next_sibling(picture) : nullptr;
  if (GTK_IS_LABEL(label) && index < MoodbarSettingsLabels::StyleChoices().size()) {
    gtk_label_set_text(GTK_LABEL(label), MoodbarSettingsLabels::StyleChoices()[index].second.c_str());
  }
  if (GTK_IS_PICTURE(picture) && index < previews->pixbufs.size() && previews->pixbufs[index]) {
    GdkTexture *texture = gdk_texture_new_for_pixbuf(previews->pixbufs[index]);
    gtk_picture_set_paintable(GTK_PICTURE(picture), GDK_PAINTABLE(texture));
    g_object_unref(texture);
  }
}

GtkWidget *CreateStyleDropDown(Settings *settings) {
  GtkStringList *model = gtk_string_list_new(nullptr);
  for (const auto &choice : MoodbarSettingsLabels::StyleChoices()) {
    gtk_string_list_append(model, choice.second.c_str());
  }
  GtkWidget *combo = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
  const int current = settings ? settings->IntValue(MoodbarSettings::kStyle, static_cast<int>(MoodbarSettings::kDefaultStyle))
                               : static_cast<int>(MoodbarSettings::kDefaultStyle);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(combo), static_cast<guint>(MoodbarStyle::ClampStyle(current)));

  auto *previews = new MoodbarPreviewPixbufs();
  const std::vector<uint8_t> sample = MoodbarPreview::LoadSample();
  for (int i = 0; i < static_cast<int>(MoodbarSettings::Style::StyleCount); ++i) {
    previews->pixbufs.push_back(PixbufFromStripe(MoodbarPreview::Stripe(sample, static_cast<MoodbarSettings::Style>(i), MoodbarPreview::kWidth)));
  }

  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_object_set_data_full(G_OBJECT(factory), "pixbufs", previews, FreePreviewPixbufs);
  g_signal_connect(factory, "setup", G_CALLBACK(PreviewSetup), nullptr);
  g_signal_connect(factory, "bind", G_CALLBACK(PreviewBind), nullptr);
  gtk_drop_down_set_factory(GTK_DROP_DOWN(combo), factory);
  gtk_drop_down_set_list_factory(GTK_DROP_DOWN(combo), factory);
  g_object_unref(factory);

  g_signal_connect(combo, "notify::selected", G_CALLBACK((+[](GtkDropDown *dropdown, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     if (!s) {
                       return;
                     }
                     s->BeginGroup(MoodbarSettings::kSettingsGroup);
                     s->SetIntValue(MoodbarSettings::kStyle, static_cast<int>(gtk_drop_down_get_selected(dropdown)));
                     s->Sync();
                   })),
                   settings);
  return combo;
}

}  // namespace

AdwPreferencesPage *MoodbarSettingsPage::Create(Settings *settings, Application *) {
  AdwPreferencesPage *page = SettingsPage::MakePage("Moodbar", "weather-clear-symbolic");
  settings->BeginGroup(SeekbarSettings::kSettingsGroup);
  AdwPreferencesGroup *seek = SettingsPage::AddGroup(page, "Seek bar");
  SettingsPage::AddIntCombo(seek, settings, SeekbarSettings::kSettingsGroup, SeekbarSettings::kMode, "Mode",
                            {{"0", SeekbarModeMenu::Label(SeekbarSettings::Mode::Normal)},
                             {"1", SeekbarModeMenu::Label(SeekbarSettings::Mode::Moodbar)},
                             {"2", SeekbarModeMenu::Label(SeekbarSettings::Mode::Waveform)}},
                            static_cast<int>(SeekbarSettings::kDefaultMode));
  settings->BeginGroup(MoodbarSettings::kSettingsGroup);
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Moodbar");
  AdwActionRow *style_row = ADW_ACTION_ROW(adw_action_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(style_row), MoodbarSettingsLabels::StyleLabel());
  GtkWidget *combo = CreateStyleDropDown(settings);
  adw_action_row_add_suffix(style_row, combo);
  adw_action_row_set_activatable_widget(style_row, combo);
  adw_preferences_group_add(group, GTK_WIDGET(style_row));
  SettingsPage::AddToggle(group, settings, MoodbarSettings::kSave, MoodbarSettingsLabels::SaveLabel(), nullptr, MoodbarSettings::kDefaultSave);
  return page;
}
