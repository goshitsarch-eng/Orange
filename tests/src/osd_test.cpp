#include "core/seekbarsettings.h"
#include "core/song.h"
#include "core/standardpaths.h"
#include "covermanager/coverfromurldialog.h"
#include "osd/osdbase.h"
#include "osd/osdpretty.h"
#include "osd/osdprettyplacement.h"
#include "queue/queueview.h"
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

TEST(OSDPretty, SupportedRequiresX11Display) {
  if (!gdk_display_get_default()) {
    EXPECT_FALSE(OSDPretty::Supported());
    EXPECT_FALSE(OSDBase::SupportsOSDPretty());
  }
}

TEST(OSDPrettyPlacement, AbsolutePositionUsesMonitorOriginAndNegativeEdges) {
  const OSDPrettyPlacement::Rect monitor{1920, 0, 1920, 1080};
  const OSDPrettyPlacement::Point abs = OSDPrettyPlacement::AbsolutePosition(monitor, {40, 40}, 320, 80, false);
  EXPECT_EQ(1960, abs.x);
  EXPECT_EQ(40, abs.y);
  const OSDPrettyPlacement::Point docked = OSDPrettyPlacement::AbsolutePosition(monitor, {-1, -1}, 320, 80, false);
  EXPECT_EQ(monitor.Right() - 320, docked.x);
  EXPECT_EQ(monitor.Bottom() - 80, docked.y);
  const OSDPrettyPlacement::Point on_second = OSDPrettyPlacement::AbsolutePosition(monitor, {40, 40}, 320, 80, true);
  EXPECT_EQ(1960, on_second.x);
  EXPECT_EQ(40, on_second.y);
  const OSDPrettyPlacement::Point clamped = OSDPrettyPlacement::AbsolutePosition({0, 0, 800, 600}, {900, 700}, 320, 80, true);
  EXPECT_EQ(479, clamped.x);
  EXPECT_EQ(519, clamped.y);
}

TEST(OSDPrettyPlacement, ResolveIndexAcceptsNameAndLegacyInt) {
  const std::vector<std::string> names{"eDP-1", "HDMI-1"};
  EXPECT_EQ(0, OSDPrettyPlacement::ResolveIndex({}, names));
  EXPECT_EQ(1, OSDPrettyPlacement::ResolveIndex("HDMI-1", names));
  EXPECT_EQ(1, OSDPrettyPlacement::ResolveIndex("1", names));
  EXPECT_EQ(0, OSDPrettyPlacement::ResolveIndex("missing", names));
  EXPECT_EQ(0, OSDPrettyPlacement::ResolveIndex("9", names));
}

TEST(OSDPrettyPlacement, RelativePosMarksDockedEdgesAndParsesSavedPoint) {
  const OSDPrettyPlacement::Rect monitor{0, 0, 1920, 1080};
  const OSDPrettyPlacement::Point docked = OSDPrettyPlacement::RelativePosition(monitor, {1600, 1000}, 320, 80);
  EXPECT_EQ(-1, docked.x);
  EXPECT_EQ(-1, docked.y);
  const OSDPrettyPlacement::Point rel = OSDPrettyPlacement::RelativePosition(monitor, {40, 50}, 320, 80);
  EXPECT_EQ(40, rel.x);
  EXPECT_EQ(50, rel.y);
  EXPECT_EQ("40,50", OSDPrettyPlacement::FormatPos({40, 50}));
  const OSDPrettyPlacement::Point parsed = OSDPrettyPlacement::ParsePos("40,50");
  EXPECT_EQ(40, parsed.x);
  EXPECT_EQ(50, parsed.y);
  const OSDPrettyPlacement::Point snapped = OSDPrettyPlacement::DragPosition(monitor, {790, 40}, 320, 80);
  EXPECT_EQ(800, snapped.x);
}

TEST(CoverFromUrlDialog, PrefillUrlAcceptsHttpOnly) {
  EXPECT_EQ("https://example.com/a.jpg", CoverFromUrlDialog::PrefillUrl(" https://example.com/a.jpg\n"));
  EXPECT_EQ("http://example.com/a.png", CoverFromUrlDialog::PrefillUrl("http://example.com/a.png"));
  EXPECT_TRUE(CoverFromUrlDialog::PrefillUrl("not a url").empty());
  EXPECT_TRUE(CoverFromUrlDialog::PrefillUrl("ftp://example.com/a.jpg").empty());
}

TEST(QueueView, NowPlayingMatchesUrl) {
  Song song;
  song.set_url("file:///tmp/roads.flac");
  EXPECT_TRUE(QueueView::IsNowPlaying(song, "file:///tmp/roads.flac"));
  EXPECT_FALSE(QueueView::IsNowPlaying(song, "file:///tmp/other.flac"));
  EXPECT_FALSE(QueueView::IsNowPlaying(song, {}));
}

TEST(SeekbarSettings, ModeValuesMatchQt) {
  EXPECT_EQ(0, static_cast<int>(SeekbarSettings::Mode::Normal));
  EXPECT_EQ(1, static_cast<int>(SeekbarSettings::Mode::Moodbar));
  EXPECT_EQ(2, static_cast<int>(SeekbarSettings::Mode::Waveform));
  EXPECT_STREQ("mode", SeekbarSettings::kMode);
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
