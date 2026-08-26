#include "core/song.h"
#include "core/standardpaths.h"
#include "osd/osdbase.h"
#include "systemtrayicon/systemtrayicon.h"
#include "translations/translations.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <fstream>
#include <string>

#include <gtest/gtest.h>
#include <unistd.h>

TEST(StrUtils, ReplaceMessageArtistTitleAndNewline) {
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_track(8);
  song.set_disc(1);
  EXPECT_EQ("Portishead - Roads", StrUtils::ReplaceMessage("%artist% - %title%", song));
  EXPECT_EQ("Portishead\nRoads", StrUtils::ReplaceMessage("%artist%%newline%%title%", song));
  EXPECT_EQ("Dummy", StrUtils::ReplaceMessage("%album%", song));
}

TEST(SystemTrayIcon, ShowPopupStoresTextWithoutGtk) {
  SystemTrayIcon tray;
  tray.ShowPopup("Portishead - Roads", "Dummy", 3500);
  EXPECT_EQ("Portishead - Roads", tray.popup_summary());
  EXPECT_EQ("Dummy", tray.popup_message());
  EXPECT_EQ(3500, tray.popup_timeout_ms());
}

namespace {

class TestOSD : public OSDBase {
 public:
  explicit TestOSD(SystemTrayIcon *tray) : OSDBase(tray) {
    enabled_ = true;
    type_ = OSDSettings::Type::TrayPopup;
    timeout_ms_ = 2500;
  }

  void EnableCustomText(const std::string &line1, const std::string &line2) {
    use_custom_text_ = true;
    custom_text1_ = line1;
    custom_text2_ = line2;
  }

  using OSDBase::PlayingBody;
  using OSDBase::PlayingSummary;
};

}  // namespace

TEST(OSDBase, TrayPopupRoutesToTrayIcon) {
  SystemTrayIcon tray;
  TestOSD osd(&tray);
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  osd.SongChanged(song);
  EXPECT_EQ("Portishead - Roads", tray.popup_summary());
  EXPECT_EQ("Dummy", tray.popup_message());
  EXPECT_EQ(2500, tray.popup_timeout_ms());
}

TEST(OSDBase, CustomTextAndPlaylistFinished) {
  SystemTrayIcon tray;
  TestOSD osd(&tray);
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  osd.EnableCustomText("%artist% - %title%", "%album%");
  song.set_album("Dummy");
  EXPECT_EQ("Portishead - Roads", osd.PlayingSummary(song));
  EXPECT_EQ("Dummy", osd.PlayingBody(song));
  osd.PlaylistFinished();
  EXPECT_EQ("Strawberry", tray.popup_summary());
  EXPECT_EQ("Playlist finished", tray.popup_message());
}

TEST(Translations, AvailableLanguagesIncludeGerman) {
  const auto languages = Translations::AvailableLanguages();
  EXPECT_NE(std::find(languages.begin(), languages.end(), "de_DE"), languages.end());
  EXPECT_EQ(31u, languages.size());
  EXPECT_FALSE(StandardPaths::LocaleDir().empty());
  EXPECT_STREQ("Preferences", Translations::CStr("Preferences"));
}

TEST(Translations, GermanCatalogContainsAbout) {
#ifdef STRAWBERRY_SOURCE_DIR
  const std::string po = std::string(STRAWBERRY_SOURCE_DIR) + "/src/translations/strawberry_de_DE.po";
  std::ifstream in(po);
  ASSERT_TRUE(in) << po;
  std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(contents.find("msgid \"About\""), std::string::npos);
  EXPECT_NE(contents.find("msgstr \"Über\""), std::string::npos);
#endif
#ifdef STRAWBERRY_LOCALE_DIR
  const std::string mo = std::string(STRAWBERRY_LOCALE_DIR) + "/de_DE/LC_MESSAGES/strawberry.mo";
  EXPECT_EQ(0, access(mo.c_str(), R_OK)) << mo;
#endif
}
