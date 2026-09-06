// Orange GUI and asynchronous radio regression tests. SPDX-License-Identifier: GPL-3.0-or-later
#include "gtest_include.h"

#include <cstring>
#include <memory>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QNetworkReply>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStyle>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QTreeView>
#include <QUrlQuery>

#include "constants/appearancesettings.h"
#include "core/appearance.h"
#include "core/mimedata.h"
#include "core/networkaccessmanager.h"
#include "core/settings.h"
#include "core/taskmanager.h"
#include "fileview/fileview.h"
#include "fileview/fileviewtree.h"
#include "radios/radiobrowsersearchview.h"
#include "radios/radiobrowserservice.h"
#include "settings/appearancesettingspage.h"
#include "settings/settingspage.h"

using namespace Qt::Literals::StringLiterals;

namespace {

class ControlledReply : public QNetworkReply {
 public:
  ControlledReply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent) {
    setRequest(request);
    setUrl(request.url());
    open(QIODevice::ReadOnly);
  }
  void abort() override {
    aborted = true;
    Finish(QByteArray(), true);
  }
  void Finish(const QByteArray &data, bool fail = false) {
    data_ = data;
    if (fail) setError(QNetworkReply::ConnectionRefusedError, u"Test connection failure"_s);
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, fail ? 503 : 200);
    setFinished(true);
    Q_EMIT readyRead();
    Q_EMIT finished();
  }
  bool aborted = false;
 protected:
  qint64 readData(char *buffer, qint64 size) override {
    const qint64 count = qMin(size, static_cast<qint64>(data_.size()));
    if (!count) return -1;
    std::memcpy(buffer, data_.constData(), static_cast<size_t>(count));
    data_.remove(0, count);
    return count;
  }
 private:
  QByteArray data_;
};

class ControlledNetwork : public NetworkAccessManager {
 public:
  QList<QPointer<ControlledReply>> requests;
 protected:
  QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *) override {
    auto *reply = new ControlledReply(request, this);
    requests << reply;
    return reply;
  }
};

QByteArray Stations(int count) {
  QJsonArray array;
  for (int i = 0; i < count; ++i) {
    array.append(QJsonObject{{u"name"_s, u"Station %1"_s.arg(i)}, {u"url"_s, u"https://example.com/radio"_s}});
  }
  return QJsonDocument(array).toJson();
}

class GuiRegressionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    QCoreApplication::setOrganizationName(u"OrangeTests"_s);
    QCoreApplication::setApplicationName(u"GuiRegressions"_s);
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, directory_.path());
    Settings().clear();
    palette_ = QApplication::palette();
  }
  void TearDown() override {
    QApplication::setPalette(palette_);
    Settings().clear();
  }
  QTemporaryDir directory_;
  QPalette palette_;
};

class NumericSettingsPage : public SettingsPage {
 public:
  NumericSettingsPage() : SettingsPage(nullptr), value(this) { Load(); }
  void Load() override { Init(this); }
  void Save() override { ++saves; }
  QDoubleSpinBox value;
  int saves = 0;
};

TEST_F(GuiRegressionsTest, RepeatedApplyDoesNotResaveUnchangedDecimalSettings) {
  NumericSettingsPage page;
  page.value.setValue(2.5);
  page.Apply();
  ASSERT_EQ(page.saves, 1);
  page.Apply();
  EXPECT_EQ(page.saves, 1);
  page.Load();
  page.Apply();
  EXPECT_EQ(page.saves, 1);
}

TEST_F(GuiRegressionsTest, AppearanceCancelPreservesAppliedPalette) {
  auto appearance = std::make_shared<Appearance>();
  appearance->set_default_style(QApplication::style()->objectName());
  AppearanceSettingsPage page(nullptr, appearance);
  ASSERT_TRUE(QMetaObject::invokeMethod(&page, "SetDarkColors", Qt::DirectConnection));
  page.Apply();
  const QPalette applied = QApplication::palette();
  auto *system = page.findChild<QRadioButton*>(u"use_system_color_set"_s);
  ASSERT_NE(system, nullptr);
  system->setChecked(true);
  page.Reject();
  EXPECT_EQ(QApplication::palette(), applied);
}

TEST_F(GuiRegressionsTest, SystemColorsRestorePaletteCapturedBeforeCustomColors) {
  auto appearance = std::make_shared<Appearance>();
  appearance->set_default_style(QApplication::style()->objectName());
  Settings settings;
  settings.beginGroup(AppearanceSettings::kSettingsGroup);
  settings.setValue(AppearanceSettings::kUseCustomColorSet, true);
  Appearance::SetCustomPaletteColors(Appearance::DarkColors());
  AppearanceSettingsPage page(nullptr, appearance);
  page.findChild<QRadioButton*>(u"use_system_color_set"_s)->setChecked(true);
  EXPECT_EQ(QApplication::palette(), appearance->system_palette());
}

TEST_F(GuiRegressionsTest, BackgroundCropRequiresStretchAndAspectRatio) {
  auto appearance = std::make_shared<Appearance>();
  appearance->set_default_style(QApplication::style()->objectName());
  AppearanceSettingsPage page(nullptr, appearance);
  page.findChild<QRadioButton*>(u"use_custom_background_image"_s)->setChecked(true);
  auto *stretch = page.findChild<QCheckBox*>(u"checkbox_background_image_stretch"_s);
  auto *ratio = page.findChild<QCheckBox*>(u"checkbox_background_image_keep_aspect_ratio"_s);
  auto *crop = page.findChild<QCheckBox*>(u"checkbox_background_image_do_not_cut"_s);
  stretch->setChecked(false);
  ratio->setChecked(false);
  stretch->setChecked(true);
  EXPECT_FALSE(crop->isEnabled());
  ratio->setChecked(true);
  EXPECT_TRUE(crop->isEnabled());
  stretch->setChecked(false);
  EXPECT_FALSE(crop->isEnabled());
  EXPECT_TRUE(page.findChild<QSpinBox*>(u"spinbox_background_image_maxsize"_s)->isEnabled());
}

TEST_F(GuiRegressionsTest, RemovingNestedRootKeepsItsParentRoot) {
  const QString parent = directory_.path();
  const QString nested = parent + u"/nested"_s;
  ASSERT_TRUE(QDir().mkpath(nested));
  Settings settings;
  settings.setValue(u"FileView/tree_root_paths"_s, QStringList{parent, nested});
  settings.setValue(u"FileView/tree_view_active"_s, true);
  FileView view;
  view.show();
  auto *tree = view.findChild<FileViewTree*>(u"tree"_s);
  ASSERT_NE(tree, nullptr);
  ASSERT_EQ(tree->model()->rowCount(), 2);
  tree->setCurrentIndex(tree->model()->index(1, 0));
  view.findChild<QToolButton*>(u"remove_tree_root"_s)->click();
  EXPECT_EQ(settings.value(u"FileView/tree_root_paths"_s).toStringList(), QStringList{parent});
}

TEST_F(GuiRegressionsTest, EnterInFileTreeAddsSelectedFileExactlyOnce) {
  const QString music = directory_.path() + u"/music"_s;
  ASSERT_TRUE(QDir().mkpath(music));
  const QString filename = music + u"/track.flac"_s;
  QFile file(filename);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.close();
  Settings().setValue(u"FileView/tree_root_paths"_s, QStringList{music});
  Settings().setValue(u"FileView/tree_view_active"_s, true);
  FileView view;
  view.show();
  auto *tree = view.findChild<FileViewTree*>(u"tree"_s);
  const QModelIndex root = tree->model()->index(0, 0);
  tree->expand(root);
  if (tree->model()->canFetchMore(root)) tree->model()->fetchMore(root);
  const QModelIndex track = tree->model()->index(0, 0, root);
  ASSERT_TRUE(track.isValid());
  tree->setCurrentIndex(track);
  int additions = 0;
  QObject::connect(&view, &FileView::AddToPlaylist, &view, [&](QMimeData *data) {
    ++additions;
    EXPECT_EQ(data->urls(), QList<QUrl>{QUrl::fromLocalFile(filename)});
    delete data;
  });
  QTest::keyClick(tree, Qt::Key_Return);
  EXPECT_EQ(additions, 1);
}

TEST_F(GuiRegressionsTest, TypedFilePathNavigatesOnlyOnEnter) {
  const QString music = directory_.path() + u"/music"_s;
  ASSERT_TRUE(QDir().mkpath(music));
  FileView view;
  view.SetPath(directory_.path());
  view.show();
  auto *path = view.findChild<QLineEdit*>(u"path"_s);
  ASSERT_NE(path, nullptr);
  QSignalSpy navigations(&view, &FileView::PathChanged);
  QSignalSpy additions(&view, &FileView::AddToPlaylist);
  path->setFocus();
  path->setText(music);
  EXPECT_EQ(navigations.count(), 0);
  QTest::keyClick(path, Qt::Key_Return);
  ASSERT_EQ(navigations.count(), 1);
  EXPECT_EQ(navigations.first().first().toString(), music);
  EXPECT_EQ(additions.count(), 0);
}

TEST_F(GuiRegressionsTest, RadioSearchCoalescesDiscoveryAndDiscardsObsoleteReply) {
  auto tasks = std::make_shared<TaskManager>();
  auto network = std::make_shared<ControlledNetwork>();
  RadioBrowserService service(tasks, network);
  QSignalSpy finished(&service, &RadioBrowserService::SearchFinished);
  service.Search(u"first"_s);
  service.Search(u"second"_s);
  ASSERT_EQ(network->requests.size(), 1);
  network->requests.last()->Finish("{}");
  ASSERT_EQ(network->requests.size(), 2);
  EXPECT_EQ(QUrlQuery(network->requests.last()->url()).queryItemValue(u"name"_s), u"second"_s);
  auto obsolete = network->requests.last();
  service.Search(u"third"_s);
  ASSERT_TRUE(obsolete->aborted);
  obsolete->Finish(Stations(1));
  EXPECT_EQ(finished.count(), 0);
  EXPECT_EQ(tasks->GetTasks().size(), 1);
  network->requests.last()->Finish(Stations(1));
  EXPECT_EQ(finished.count(), 1);
  EXPECT_TRUE(tasks->GetTasks().isEmpty());
}

TEST_F(GuiRegressionsTest, RadioPaginationRetriesFailedPageAndBlocksDoubleClick) {
  Settings().setValue(u"RadioBrowser/search_limit"_s, 10);
  auto tasks = std::make_shared<TaskManager>();
  auto network = std::make_shared<ControlledNetwork>();
  RadioBrowserService service(tasks, network);
  RadioBrowserSearchView view;
  view.Init(&service);
  ASSERT_TRUE(QMetaObject::invokeMethod(&view, "SearchTriggered", Qt::DirectConnection));
  network->requests.last()->Finish("{}");
  network->requests.last()->Finish(Stations(10));
  ASSERT_TRUE(QMetaObject::invokeMethod(&view, "LoadMore", Qt::DirectConnection));
  EXPECT_EQ(QUrlQuery(network->requests.last()->url()).queryItemValue(u"offset"_s), u"10"_s);
  const qsizetype count = network->requests.size();
  ASSERT_TRUE(QMetaObject::invokeMethod(&view, "LoadMore", Qt::DirectConnection));
  EXPECT_EQ(network->requests.size(), count);
  network->requests.last()->Finish(QByteArray(), true);
  ASSERT_TRUE(QMetaObject::invokeMethod(&view, "LoadMore", Qt::DirectConnection));
  network->requests.last()->Finish("{}");  // Rediscover after the connection failure.
  EXPECT_EQ(QUrlQuery(network->requests.last()->url()).queryItemValue(u"offset"_s), u"10"_s);
  network->requests.last()->Finish(Stations(1));
  EXPECT_EQ(view.findChild<QTreeView*>(u"results"_s)->model()->rowCount(), 11);
}

TEST_F(GuiRegressionsTest, RadioSettingsReloadWithoutRestartAndCountryLoadDoesNotSearch) {
  auto tasks = std::make_shared<TaskManager>();
  auto network = std::make_shared<ControlledNetwork>();
  RadioBrowserService service(tasks, network);
  RadioBrowserSearchView view;
  view.Init(&service);
  const auto before = network->requests.size();
  service.CountriesLoaded({{u"United States"_s, u"US"_s}});
  EXPECT_EQ(network->requests.size(), before);
  Settings().setValue(u"RadioBrowser/search_limit"_s, 20);
  Settings().setValue(u"RadioBrowser/default_country"_s, u"US"_s);
  Settings().setValue(u"RadioBrowser/default_sort"_s, u"name"_s);
  Settings().setValue(u"RadioBrowser/hide_broken"_s, false);
  view.ReloadSettings();
  network->requests.last()->Finish("{}");
  const QUrlQuery query(network->requests.last()->url());
  EXPECT_EQ(query.queryItemValue(u"limit"_s), u"20"_s);
  EXPECT_EQ(query.queryItemValue(u"countrycode"_s), u"US"_s);
  EXPECT_EQ(query.queryItemValue(u"order"_s), u"name"_s);
  EXPECT_FALSE(query.hasQueryItem(u"hidebroken"_s));
}

}  // namespace
