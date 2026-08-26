cmake_minimum_required(VERSION 3.13)

project(strawberry C CXX)

include(FindPkgConfig)
include(GNUInstallDirs)
include(cmake/Version.cmake)
include(cmake/OptionalComponent.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_WERROR "Build with -Werror" OFF)
option(ENABLE_DEBUG_OUTPUT "Enable debug output" ON)
option(USE_INSTALL_PREFIX "Look for data in CMAKE_INSTALL_PREFIX" ON)
option(INSTALL_TRANSLATIONS "Install translations" OFF)

if(BUILD_WERROR)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost)
if(NOT Boost_FOUND)
  find_package(Boost REQUIRED)
endif()
find_package(ICU COMPONENTS uc i18n REQUIRED)

pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GOBJECT REQUIRED IMPORTED_TARGET gobject-2.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(ADWAITA REQUIRED IMPORTED_TARGET libadwaita-1)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_AUDIO REQUIRED IMPORTED_TARGET gstreamer-audio-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_TAG REQUIRED IMPORTED_TARGET gstreamer-tag-1.0)
pkg_check_modules(GSTREAMER_PBUTILS REQUIRED IMPORTED_TARGET gstreamer-pbutils-1.0)
pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3>=3.9)
pkg_check_modules(SOUP REQUIRED IMPORTED_TARGET libsoup-3.0)
pkg_check_modules(JSONGLIB REQUIRED IMPORTED_TARGET json-glib-1.0)
pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)

if(LINUX)
  find_package(ALSA)
  pkg_check_modules(LIBPULSE IMPORTED_TARGET libpulse)
  pkg_check_modules(GIO_UNIX IMPORTED_TARGET gio-unix-2.0)
endif()

pkg_check_modules(LIBCDIO IMPORTED_TARGET libcdio)
pkg_check_modules(CHROMAPRINT IMPORTED_TARGET libchromaprint>=1.4)
pkg_check_modules(FFTW3 IMPORTED_TARGET fftw3)
pkg_check_modules(LIBEBUR128 IMPORTED_TARGET libebur128)
pkg_check_modules(LIBGPOD IMPORTED_TARGET libgpod-1.0)
pkg_check_modules(LIBMTP IMPORTED_TARGET libmtp>=1.0)

find_package(TagLib 2.0)
if(TARGET TagLib::TagLib)
  set(TAGLIB_FOUND ON)
  set(TAGLIB_LIBRARIES TagLib::TagLib)
  set(HAVE_TAGLIB_DSFFILE ON)
  set(HAVE_TAGLIB_DSDIFFFILE ON)
else()
  pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib>=1.12)
  set(TAGLIB_LIBRARIES PkgConfig::TAGLIB)
endif()

find_package(GTest)

if(LINUX)
  optional_component(ALSA ON "ALSA integration" DEPENDS "alsa" ALSA_FOUND)
  optional_component(PULSE ON "PulseAudio integration" DEPENDS "libpulse" LIBPULSE_FOUND)
  optional_component(MPRIS2 ON "MPRIS2 D-Bus interface")
  optional_component(UDISKS2 ON "Devices: UDisks2 backend")
  optional_component(GIO ON "Devices: GIO device backend" DEPENDS "libgio" GIO_FOUND)
  optional_component(GIO_UNIX ON "Devices: GIO Unix backend" DEPENDS "libgio-unix" GIO_UNIX_FOUND)
endif()

optional_component(CHROMAPRINT ON "Chromaprint" DEPENDS "chromaprint" CHROMAPRINT_FOUND)
optional_component(SONGTRACKING ON "Song tracking" DEPENDS "chromaprint" HAVE_CHROMAPRINT)
optional_component(TAGFETCHER ON "Tag fetcher" DEPENDS "chromaprint" HAVE_CHROMAPRINT)
optional_component(AUDIOCD ON "Devices: Audio CD support" DEPENDS "libcdio" LIBCDIO_FOUND)
optional_component(MTP ON "Devices: MTP support" DEPENDS "libmtp" LIBMTP_FOUND)
optional_component(GPOD ON "Devices: iPod classic support" DEPENDS "libgpod" LIBGPOD_FOUND)
optional_component(MOODBAR ON "Moodbar")
optional_component(GSTFASTSPECTRUM ON "Moodbar: Fast spectrum with FFTW3"
  DEPENDS "moodbar" HAVE_MOODBAR
  DEPENDS "fftw3" FFTW3_FOUND
)
optional_component(WAVEFORM ON "Waveform")
optional_component(EBUR128 ON "EBU R 128 loudness normalization" DEPENDS "libebur128" LIBEBUR128_FOUND)
optional_component(SUBSONIC ON "Streaming: Subsonic")
optional_component(TIDAL ON "Streaming: Tidal")
optional_component(SPOTIFY ON "Streaming: Spotify")
optional_component(QOBUZ ON "Streaming: Qobuz")
optional_component(DISCORD_RPC ON "Discord Rich Presence")
optional_component(GLOBALSHORTCUTS ON "Global keyboard shortcuts")
optional_component(TRANSLATIONS OFF "Translations")

if(LINUX)
  find_package(X11)
  if(X11_FOUND)
    set(HAVE_X11 ON)
  endif()
endif()

set(HAVE_DBUS ON)
set(HAVE_GTK4 ON)
set(HAVE_ADWAITA ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/version.h)

find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)
set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/data/strawberry.gresource.xml)
set(GRESOURCE_C ${CMAKE_CURRENT_BINARY_DIR}/strawberry-resources.c)
add_custom_command(
  OUTPUT ${GRESOURCE_C}
  COMMAND ${GLIB_COMPILE_RESOURCES}
          --target=${GRESOURCE_C}
          --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/data
          --generate-source
          ${GRESOURCE_XML}
  DEPENDS ${GRESOURCE_XML} ${CMAKE_CURRENT_SOURCE_DIR}/data/schema/schema.sql
  VERBATIM
)

set(STRAWBERRY_SOURCES
  src/main.cpp
  src/core/logging.cpp
  src/core/standardpaths.cpp
  src/core/settings.cpp
  src/core/song.cpp
  src/core/database.cpp
  src/core/taskmanager.cpp
  src/core/network.cpp
  src/core/commandlineoptions.cpp
  src/core/application.cpp
  src/core/urlhandlers.cpp
  src/core/oauthenticator.cpp
  src/core/songloader.cpp
  src/utilities/timeutils.cpp
  src/utilities/fileutils.cpp
  src/utilities/strutils.cpp
  src/utilities/jsonutils.cpp
  src/utilities/musixmatchprovider.cpp
  src/utilities/audioanalysis.cpp
  src/engine/gstengine.cpp
  src/engine/gstenginepipeline.cpp
  src/engine/devicefinders.cpp
  src/core/player.cpp
  src/tagreader/tagreader.cpp
  src/tagreader/tagreadergme.cpp
  src/tagreader/streamtagreader.cpp
  src/tagreader/tagreaderresult.cpp
  src/tagreader/tagreaderclient.cpp
  src/collection/collectionbackend.cpp
  src/collection/collectionwatcher.cpp
  src/collection/collectionlibrary.cpp
  src/collection/collectiongrouping.cpp
  src/collection/collectionfilteroptions.cpp
  src/collection/collectionquery.cpp
  src/collection/collectionitem.cpp
  src/collection/collectionfilter.cpp
  src/collection/collectionmodel.cpp
  src/collection/collectionplaylistitem.cpp
  src/collection/collectiondirectorymodel.cpp
  src/collection/collectiontask.cpp
  src/collection/collectionitemdelegate.cpp
  src/collection/collectionfilterwidget.cpp
  src/collection/collectionview.cpp
  src/collection/collectionviewcontainer.cpp
  src/collection/groupbydialog.cpp
  src/collection/savedgroupingmanager.cpp
  src/playlist/playlist.cpp
  src/playlist/playlistitem.cpp
  src/playlist/songplaylistitem.cpp
  src/playlist/streamplaylistitem.cpp
  src/playlist/playlistitemmimedata.cpp
  src/playlist/playlistitemsavedata.cpp
  src/playlist/playlistfilter.cpp
  src/playlist/playlistsequence.cpp
  src/playlist/playlistundocommandbase.cpp
  src/playlist/playlistdelegates.cpp
  src/playlist/playlistheader.cpp
  src/playlist/playlistview.cpp
  src/playlist/playlisttabbar.cpp
  src/playlist/playlistcontainer.cpp
  src/playlist/playlistlistmodel.cpp
  src/playlist/playlistlistsortfiltermodel.cpp
  src/playlist/playlistlistview.cpp
  src/playlist/playlistlistcontainer.cpp
  src/playlist/songloaderinserter.cpp
  src/playlist/dynamicplaylistcontrols.cpp
  src/playlist/playlistsaveoptionsdialog.cpp
  src/playlist/playlistcolumnsdialog.cpp
  src/fileview/fileviewtreemodel.cpp
  src/fileview/fileviewlist.cpp
  src/fileview/fileviewtree.cpp
  src/fileview/fileview.cpp
  src/playlist/playlistbackend.cpp
  src/playlist/playlistmanager.cpp
  src/widgets/ratingpainter.cpp
  src/widgets/ratingwidget.cpp
  src/widgets/sliderslider.cpp
  src/widgets/stickyslider.cpp
  src/widgets/prettyslider.cpp
  src/widgets/trackslider.cpp
  src/widgets/volumeslider.cpp
  src/widgets/favoritewidget.cpp
  src/widgets/playingwidget.cpp
  src/widgets/loginstatewidget.cpp
  src/widgets/busyindicator.cpp
  src/widgets/searchfield.cpp
  src/widgets/clickablelabel.cpp
  src/widgets/freespacebar.cpp
  src/widgets/groupediconview.cpp
  src/widgets/multiloadingindicator.cpp
  src/context/contextalbum.cpp
  src/context/contextview.cpp
  src/queue/queue.cpp
  src/playlistparsers/parserbase.cpp
  src/playlistparsers/m3uparser.cpp
  src/playlistparsers/plsparser.cpp
  src/playlistparsers/xspfparser.cpp
  src/playlistparsers/asxparser.cpp
  src/playlistparsers/asxiniparser.cpp
  src/playlistparsers/wplparser.cpp
  src/playlistparsers/cueparser.cpp
  src/playlistparsers/playlistparser.cpp
  src/smartplaylists/smartplaylist.cpp
  src/smartplaylists/smartplaylistwizard.cpp
  src/filterparser/filterparser.cpp
  src/covermanager/coverproviders.cpp
  src/covermanager/coversearchstatistics.cpp
  src/covermanager/albumcoverloaderoptions.cpp
  src/covermanager/albumcoverloader.cpp
  src/covermanager/currentalbumcoverloader.cpp
  src/covermanager/albumcoverfetchersearch.cpp
  src/covermanager/albumcoverfetcher.cpp
  src/covermanager/coverexportrunnable.cpp
  src/covermanager/albumcoverexporter.cpp
  src/covermanager/albumcovermanager.cpp
  src/covermanager/albumcoversearcher.cpp
  src/covermanager/albumcoverexportdialog.cpp
  src/covermanager/coverfromurldialog.cpp
  src/covermanager/albumcoverchoicecontroller.cpp
  src/covermanager/coversearchstatisticsdialog.cpp
  src/covermanager/lastfmcoverprovider.cpp
  src/covermanager/musicbrainzcoverprovider.cpp
  src/covermanager/deezercoverprovider.cpp
  src/covermanager/discogscoverprovider.cpp
  src/covermanager/musixmatchcoverprovider.cpp
  src/covermanager/opentidalcoverprovider.cpp
  src/covermanager/tidalcoverprovider.cpp
  src/covermanager/spotifycoverprovider.cpp
  src/covermanager/qobuzcoverprovider.cpp
  src/lyrics/htmllyricsprovider.cpp
  src/lyrics/azlyricscomlyricsprovider.cpp
  src/lyrics/elyricsnetlyricsprovider.cpp
  src/lyrics/letraslyricsprovider.cpp
  src/lyrics/songlyricscomlyricsprovider.cpp
  src/lyrics/ovhlyricsprovider.cpp
  src/lyrics/lrcliblyricsprovider.cpp
  src/lyrics/musixmatchlyricsprovider.cpp
  src/lyrics/geniuslyricsprovider.cpp
  src/lyrics/lyricsproviders.cpp
  src/scrobbler/audioscrobbler.cpp
  src/scrobbler/scrobblercache.cpp
  src/scrobbler/lastfmscrobbler.cpp
  src/scrobbler/listenbrainzscrobbler.cpp
  src/scrobbler/subsonicscrobbler.cpp
  src/streaming/streamingservices.cpp
  src/streaming/streamingsearchmodel.cpp
  src/streaming/streamingsearchsortmodel.cpp
  src/streaming/streamingsearchitemdelegate.cpp
  src/streaming/streamserviceplaylistitem.cpp
  src/streaming/streamingcollectionview.cpp
  src/streaming/streamingcollectionviewcontainer.cpp
  src/streaming/streamingsearchview.cpp
  src/streaming/streamingtabsview.cpp
  src/streaming/streamingsongsview.cpp
  src/subsonic/subsonicservice.cpp
  src/subsonic/subsonicfavoriterequest.cpp
  src/subsonic/subsonicrequest.cpp
  src/subsonic/subsonicurlhandler.cpp
  src/tidal/tidalservice.cpp
  src/tidal/tidalfavoriterequest.cpp
  src/tidal/tidalrequest.cpp
  src/tidal/tidalstreamurlrequest.cpp
  src/tidal/tidalurlhandler.cpp
  src/spotify/spotifyservice.cpp
  src/spotify/spotifyfavoriterequest.cpp
  src/spotify/spotifyrequest.cpp
  src/spotify/spotifymetadatarequest.cpp
  src/qobuz/qobuzservice.cpp
  src/qobuz/qobuzfavoriterequest.cpp
  src/qobuz/qobuzrequest.cpp
  src/qobuz/qobuzstreamurlrequest.cpp
  src/qobuz/qobuzurlhandler.cpp
  src/qobuz/qobuzmetadatarequest.cpp
  src/radios/radioservices.cpp
  src/radios/somafmservice.cpp
  src/radios/radioparadiseservice.cpp
  src/radios/radiobrowserservice.cpp
  src/device/devicemanager.cpp
  src/device/devicedatabasebackend.cpp
  src/device/mtpconnection.cpp
  src/device/mtploader.cpp
  src/device/mtpdevice.cpp
  src/device/gpodloader.cpp
  src/device/gpoddevice.cpp
  src/equalizer/equalizer.cpp
  src/equalizer/equalizerdialog.cpp
  src/analyzer/analyzer.cpp
  src/moodbar/moodbar.cpp
  src/waveform/waveform.cpp
  src/transcoder/transcoder.cpp
  src/transcoder/transcodedialog.cpp
  src/organize/organize.cpp
  src/organize/organizedialog.cpp
  src/osd/osd.cpp
  src/systemtrayicon/systemtrayicon.cpp
  src/mpris2/mpris2.cpp
  src/globalshortcuts/globalshortcuts.cpp
  src/globalshortcuts/globalshortcutgrabber.cpp
  src/discord/discord.cpp
  src/tagfetcher/tagfetcher.cpp
  src/ui/mainwindow.cpp
  src/ui/settingsdialog.cpp
  src/ui/dialogs.cpp
  src/dialogs/dialoghelpers.cpp
  src/dialogs/addstreamdialog.cpp
  src/dialogs/userpassdialog.cpp
  src/dialogs/errordialog.cpp
  src/dialogs/messagedialog.cpp
  src/dialogs/console.cpp
  src/dialogs/saveplaylistsdialog.cpp
  src/dialogs/deleteconfirmationdialog.cpp
  src/dialogs/edittagdialog.cpp
  src/dialogs/trackselectiondialog.cpp
  src/dialogs/shortcutsdialog.cpp
  src/device/copytodevicedialog.cpp
  src/settings/settingspage.cpp
  src/settings/behavioursettingspage.cpp
  src/settings/collectionsettingspage.cpp
  src/settings/backendsettingspage.cpp
  src/settings/playlistsettingspage.cpp
  src/settings/scrobblersettingspage.cpp
  src/settings/coverssettingspage.cpp
  src/settings/lyricssettingspage.cpp
  src/settings/transcodersettingspage.cpp
  src/settings/networkproxysettingspage.cpp
  src/settings/appearancesettingspage.cpp
  src/settings/contextsettingspage.cpp
  src/settings/notificationssettingspage.cpp
  src/settings/globalshortcutssettingspage.cpp
  src/settings/moodbarsettingspage.cpp
  src/settings/waveformsettingspage.cpp
  src/settings/subsonicsettingspage.cpp
  src/settings/tidalsettingspage.cpp
  src/settings/qobuzsettingspage.cpp
  src/settings/spotifysettingspage.cpp
  src/settings/radiosettingspage.cpp
  ${GRESOURCE_C}
)

add_executable(strawberry ${STRAWBERRY_SOURCES})

target_include_directories(strawberry PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(strawberry PRIVATE
  Threads::Threads
  PkgConfig::GLIB
  PkgConfig::GOBJECT
  PkgConfig::GIO
  PkgConfig::GTK4
  PkgConfig::ADWAITA
  PkgConfig::GSTREAMER
  PkgConfig::GSTREAMER_BASE
  PkgConfig::GSTREAMER_AUDIO
  PkgConfig::GSTREAMER_APP
  PkgConfig::GSTREAMER_TAG
  PkgConfig::GSTREAMER_PBUTILS
  PkgConfig::SQLITE
  PkgConfig::SOUP
  PkgConfig::JSONGLIB
  ${TAGLIB_LIBRARIES}
  ICU::uc
  ICU::i18n
)

if(HAVE_PULSE)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBPULSE)
endif()
if(HAVE_ALSA)
  target_link_libraries(strawberry PRIVATE ${ALSA_LIBRARIES})
endif()
if(HAVE_CHROMAPRINT)
  target_link_libraries(strawberry PRIVATE PkgConfig::CHROMAPRINT)
endif()
if(HAVE_EBUR128)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBEBUR128)
endif()
if(HAVE_GSTFASTSPECTRUM)
  target_link_libraries(strawberry PRIVATE PkgConfig::FFTW3)
endif()
if(HAVE_MTP)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBMTP)
endif()
if(HAVE_GPOD)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBGPOD)
endif()
if(HAVE_AUDIOCD)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBCDIO)
endif()
if(LIBSECRET_FOUND)
  target_link_libraries(strawberry PRIVATE PkgConfig::LIBSECRET)
  target_compile_definitions(strawberry PRIVATE HAVE_LIBSECRET)
endif()
if(HAVE_X11)
  target_include_directories(strawberry PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(strawberry PRIVATE ${X11_LIBRARIES})
endif()

install(TARGETS strawberry RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES dist/unix/org.strawberrymusicplayer.strawberry.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES dist/unix/org.strawberrymusicplayer.strawberry.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

if(GTest_FOUND)
  enable_testing()
  add_executable(strawberry_tests EXCLUDE_FROM_ALL
    tests/src/main.cpp
    tests/src/utilities_test.cpp
    tests/src/song_test.cpp
    tests/src/database_test.cpp
    tests/src/m3uparser_test.cpp
    tests/src/smartplaylist_test.cpp
    tests/src/filterparser_test.cpp
    tests/src/lyrics_test.cpp
    tests/src/gpod_test.cpp
    tests/src/mtp_test.cpp
    tests/src/streaming_test.cpp
    tests/src/streamingfavorite_test.cpp
    tests/src/streamingrequest_test.cpp
    tests/src/streamurl_test.cpp
    tests/src/streamingview_test.cpp
    tests/src/collection_test.cpp
    tests/src/playlistitem_test.cpp
    tests/src/playlistview_test.cpp
    tests/src/radio_test.cpp
    tests/src/tagreader_gme_test.cpp
    tests/src/streamtagreader_test.cpp
    tests/src/tagreaderclient_test.cpp
    tests/src/lastfmcover_test.cpp
    tests/src/coverprovider_test.cpp
    tests/src/albumcover_test.cpp
    tests/src/songloader_test.cpp
    tests/src/fileview_test.cpp
    tests/src/scrobbler_test.cpp
    tests/src/playlist_test.cpp
    tests/src/settings_test.cpp
    tests/src/settingspage_test.cpp
    tests/src/jsonutils_test.cpp
    src/core/logging.cpp
    src/core/standardpaths.cpp
    src/core/settings.cpp
    src/core/song.cpp
    src/core/database.cpp
    src/core/taskmanager.cpp
    src/core/network.cpp
    src/core/oauthenticator.cpp
    src/core/songloader.cpp
    src/core/urlhandlers.cpp
    src/utilities/timeutils.cpp
    src/utilities/fileutils.cpp
    src/utilities/strutils.cpp
    src/utilities/musixmatchprovider.cpp
    src/utilities/jsonutils.cpp
    src/utilities/audioanalysis.cpp
    src/device/devicemanager.cpp
    src/device/devicedatabasebackend.cpp
    src/device/mtpconnection.cpp
    src/device/mtploader.cpp
    src/device/mtpdevice.cpp
    src/device/gpodloader.cpp
    src/device/gpoddevice.cpp
    src/playlistparsers/parserbase.cpp
    src/playlistparsers/m3uparser.cpp
    src/playlistparsers/plsparser.cpp
    src/playlistparsers/xspfparser.cpp
    src/playlistparsers/asxparser.cpp
    src/playlistparsers/asxiniparser.cpp
    src/playlistparsers/wplparser.cpp
    src/playlistparsers/cueparser.cpp
    src/playlistparsers/playlistparser.cpp
    src/smartplaylists/smartplaylist.cpp
    src/filterparser/filterparser.cpp
    src/lyrics/htmllyricsprovider.cpp
    src/subsonic/subsonicservice.cpp
    src/subsonic/subsonicfavoriterequest.cpp
    src/subsonic/subsonicrequest.cpp
    src/subsonic/subsonicurlhandler.cpp
    src/streaming/streamingsearchmodel.cpp
    src/streaming/streamingsearchsortmodel.cpp
    src/streaming/streamingsearchitemdelegate.cpp
    src/streaming/streamserviceplaylistitem.cpp
    src/radios/somafmservice.cpp
    src/radios/radioparadiseservice.cpp
    src/radios/radiobrowserservice.cpp
    src/covermanager/coversearchstatistics.cpp
    src/covermanager/albumcoverloaderoptions.cpp
    src/covermanager/albumcoverloader.cpp
    src/covermanager/currentalbumcoverloader.cpp
    src/covermanager/albumcoverfetchersearch.cpp
    src/covermanager/albumcoverfetcher.cpp
    src/covermanager/coverexportrunnable.cpp
    src/covermanager/albumcoverexporter.cpp
    src/covermanager/lastfmcoverprovider.cpp
    src/covermanager/musicbrainzcoverprovider.cpp
    src/covermanager/deezercoverprovider.cpp
    src/covermanager/discogscoverprovider.cpp
    src/covermanager/musixmatchcoverprovider.cpp
    src/covermanager/opentidalcoverprovider.cpp
    src/covermanager/tidalcoverprovider.cpp
    src/covermanager/spotifycoverprovider.cpp
    src/covermanager/qobuzcoverprovider.cpp
    src/scrobbler/scrobblercache.cpp
    src/scrobbler/lastfmscrobbler.cpp
    src/scrobbler/listenbrainzscrobbler.cpp
    src/scrobbler/subsonicscrobbler.cpp
    src/tidal/tidalservice.cpp
    src/tidal/tidalfavoriterequest.cpp
    src/tidal/tidalrequest.cpp
    src/tidal/tidalstreamurlrequest.cpp
    src/tidal/tidalurlhandler.cpp
    src/spotify/spotifyservice.cpp
    src/spotify/spotifyfavoriterequest.cpp
    src/spotify/spotifyrequest.cpp
    src/spotify/spotifymetadatarequest.cpp
    src/qobuz/qobuzservice.cpp
    src/qobuz/qobuzfavoriterequest.cpp
    src/qobuz/qobuzrequest.cpp
    src/qobuz/qobuzstreamurlrequest.cpp
    src/qobuz/qobuzurlhandler.cpp
    src/qobuz/qobuzmetadatarequest.cpp
    src/tagreader/tagreadergme.cpp
    src/tagreader/tagreader.cpp
    src/tagreader/streamtagreader.cpp
    src/tagreader/tagreaderresult.cpp
    src/tagreader/tagreaderclient.cpp
    src/playlist/playlist.cpp
    src/playlist/playlistitem.cpp
    src/playlist/songplaylistitem.cpp
    src/playlist/streamplaylistitem.cpp
    src/playlist/playlistitemmimedata.cpp
    src/playlist/playlistitemsavedata.cpp
    src/playlist/playlistfilter.cpp
    src/playlist/playlistsequence.cpp
    src/playlist/playlistundocommandbase.cpp
    src/playlist/playlistdelegates.cpp
    src/playlist/playlistlistmodel.cpp
    src/playlist/playlistlistsortfiltermodel.cpp
    src/playlist/playlistsaveoptionsdialog.cpp
    src/playlist/songloaderinserter.cpp
    src/fileview/fileviewtreemodel.cpp
    src/covermanager/coversearchstatisticsdialog.cpp
    src/widgets/ratingpainter.cpp
    src/collection/collectionbackend.cpp
    src/collection/collectiongrouping.cpp
    src/collection/collectionfilteroptions.cpp
    src/collection/collectionquery.cpp
    src/collection/collectionitem.cpp
    src/collection/collectionfilter.cpp
    src/collection/collectionmodel.cpp
    src/collection/collectionplaylistitem.cpp
    src/collection/collectiondirectorymodel.cpp
    src/collection/collectiontask.cpp
    src/collection/collectionitemdelegate.cpp
    ${GRESOURCE_C}
    src/equalizer/equalizer.cpp
    src/organize/organize.cpp
    src/analyzer/analyzer.cpp
  )
  target_include_directories(strawberry_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(strawberry_tests PRIVATE
    GTest::gtest
    PkgConfig::GLIB
    PkgConfig::GOBJECT
    PkgConfig::GIO
    PkgConfig::GTK4
    PkgConfig::ADWAITA
    PkgConfig::SQLITE
    PkgConfig::JSONGLIB
    PkgConfig::SOUP
    PkgConfig::GSTREAMER
    PkgConfig::GSTREAMER_APP
    ${TAGLIB_LIBRARIES}
    ICU::uc
    ICU::i18n
  )
  if(HAVE_MTP)
    target_link_libraries(strawberry_tests PRIVATE PkgConfig::LIBMTP)
  endif()
  if(HAVE_GPOD)
    target_link_libraries(strawberry_tests PRIVATE PkgConfig::LIBGPOD)
  endif()
  if(HAVE_AUDIOCD)
    target_link_libraries(strawberry_tests PRIVATE PkgConfig::LIBCDIO)
  endif()
  add_test(NAME strawberry_tests COMMAND strawberry_tests)
  add_custom_target(build_tests DEPENDS strawberry_tests)
  add_custom_target(run_strawberry_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS strawberry_tests
  )
endif()

optional_component_summary_show()
