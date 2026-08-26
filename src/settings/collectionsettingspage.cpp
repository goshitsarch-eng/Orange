#include "settings/collectionsettingspage.h"

#include "constants/collectionsettings.h"
#include "collection/collectionbackend.h"
#include "core/application.h"
#include "settings/settingspage.h"

AdwPreferencesPage *CollectionSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(CollectionSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Collection", "media-optical-cd-audio-symbolic");
  AdwPreferencesGroup *scan = SettingsPage::AddGroup(page, "Scanning");
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kStartupScan, "Scan collection on startup", nullptr,
                          CollectionSettings::kDefaultStartupScan);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kMonitor, "Watch folders for changes", nullptr, CollectionSettings::kDefaultMonitor);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongTracking, "Track songs with Chromaprint / AcoustID", nullptr,
                          CollectionSettings::kDefaultSongTracking);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kMarkSongsUnavailable, "Mark missing songs as unavailable", nullptr,
                          CollectionSettings::kDefaultMarkSongsUnavailable);
  SettingsPage::AddToggle(scan, settings, CollectionSettings::kSongENUR128LoudnessAnalysis, "EBU R128 loudness analysis", nullptr,
                          CollectionSettings::kDefaultSongENUR128LoudnessAnalysis);
  SettingsPage::AddIntEntry(scan, settings, CollectionSettings::kExpireUnavailableSongs, "Expire unavailable songs (days)",
                            CollectionSettings::kDefaultExpireUnavailableSongs);
  SettingsPage::AddEntry(scan, settings, CollectionSettings::kCoverArtPatterns, "Cover art filename patterns", "cover.jpg,folder.jpg,front.jpg,album.jpg");

  AdwPreferencesGroup *display = SettingsPage::AddGroup(page, "Display");
  SettingsPage::AddToggle(display, settings, CollectionSettings::kAutoOpen, "Auto-open collection items", nullptr, CollectionSettings::kDefaultAutoOpen);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kShowDividers, "Show artist / album dividers", nullptr,
                          CollectionSettings::kDefaultShowDividers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kPrettyCovers, "Use pretty covers", nullptr, CollectionSettings::kDefaultPrettyCovers);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kVariousArtists, "Group various artists albums", nullptr,
                          CollectionSettings::kDefaultVariousArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForArtists, "Skip “The / A / An” when sorting artists", nullptr,
                          CollectionSettings::kDefaultSkipArticlesForArtists);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kSkipArticlesForAlbums, "Skip “The / A / An” when sorting albums", nullptr,
                          CollectionSettings::kDefaultSkipArticlesForAlbums);
  SettingsPage::AddToggle(display, settings, CollectionSettings::kUseSortTags, "Use sort tags", nullptr, CollectionSettings::kDefaultUseSortTags);

  AdwPreferencesGroup *cache = SettingsPage::AddGroup(page, "Cache");
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsCacheSize, "Icon cache size", CollectionSettings::kSettingsCacheSizeDefault);
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsCacheSizeUnit, "Icon cache unit (0 KB / 1 MB / 2 GB / 3 TB)",
                            static_cast<int>(CollectionSettings::kDefaultSettingsCacheSizeUnit));
  SettingsPage::AddToggle(cache, settings, CollectionSettings::kSettingsDiskCacheEnable, "Enable disk cache", nullptr,
                          CollectionSettings::kDefaultSettingsDiskCacheEnable);
  SettingsPage::AddIntEntry(cache, settings, CollectionSettings::kSettingsDiskCacheSize, "Disk cache size",
                            CollectionSettings::kSettingsDiskCacheSizeDefault);

  AdwPreferencesGroup *tags = SettingsPage::AddGroup(page, "Tags");
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSavePlayCounts, "Save playcounts to files", nullptr,
                          CollectionSettings::kDefaultSavePlayCounts);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kSaveRatings, "Save ratings to files", nullptr, CollectionSettings::kDefaultSaveRatings);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwritePlaycount, "Overwrite existing playcounts", nullptr,
                          CollectionSettings::kDefaultOverwritePlaycount);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kOverwriteRating, "Overwrite existing ratings", nullptr,
                          CollectionSettings::kDefaultOverwriteRating);
  SettingsPage::AddToggle(tags, settings, CollectionSettings::kDeleteFiles, "Allow deleting files from collection", nullptr,
                          CollectionSettings::kDefaultDeleteFiles);

  AdwPreferencesGroup *dirs = SettingsPage::AddGroup(page, "Folders");
  if (app) {
    for (const CollectionDirectory &directory : app->collection()->backend()->Directories()) {
      AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), directory.path.c_str());
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), directory.subdirs ? "Including subfolders" : "This folder only");
      adw_preferences_group_add(dirs, GTK_WIDGET(row));
    }
  }
  return page;
}
