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
  src/utilities/timeutils.cpp
  src/utilities/fileutils.cpp
  src/utilities/strutils.cpp
  src/engine/gstengine.cpp
  src/engine/devicefinders.cpp
  src/core/player.cpp
  src/tagreader/tagreader.cpp
  src/collection/collectionbackend.cpp
  src/collection/collectionwatcher.cpp
  src/collection/collectionlibrary.cpp
  src/playlist/playlist.cpp
  src/playlist/playlistbackend.cpp
  src/playlist/playlistmanager.cpp
  src/queue/queue.cpp
  src/playlistparsers/playlistparser.cpp
  src/smartplaylists/smartplaylist.cpp
  src/filterparser/filterparser.cpp
  src/covermanager/coverproviders.cpp
  src/lyrics/lyricsproviders.cpp
  src/scrobbler/audioscrobbler.cpp
  src/streaming/streamingservices.cpp
  src/radios/radioservices.cpp
  src/device/devicemanager.cpp
  src/equalizer/equalizer.cpp
  src/analyzer/analyzer.cpp
  src/moodbar/moodbar.cpp
  src/waveform/waveform.cpp
  src/transcoder/transcoder.cpp
  src/organize/organize.cpp
  src/osd/osd.cpp
  src/systemtrayicon/systemtrayicon.cpp
  src/mpris2/mpris2.cpp
  src/globalshortcuts/globalshortcuts.cpp
  src/discord/discord.cpp
  src/tagfetcher/tagfetcher.cpp
  src/ui/mainwindow.cpp
  src/ui/settingsdialog.cpp
  src/ui/dialogs.cpp
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
    tests/src/playlist_test.cpp
    tests/src/settings_test.cpp
    src/core/logging.cpp
    src/core/standardpaths.cpp
    src/core/settings.cpp
    src/core/song.cpp
    src/core/database.cpp
    src/utilities/timeutils.cpp
    src/utilities/fileutils.cpp
    src/utilities/strutils.cpp
    src/playlistparsers/playlistparser.cpp
    src/smartplaylists/smartplaylist.cpp
    src/filterparser/filterparser.cpp
    src/playlist/playlist.cpp
    src/collection/collectionbackend.cpp
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
    PkgConfig::SQLITE
    ${TAGLIB_LIBRARIES}
  )
  add_test(NAME strawberry_tests COMMAND strawberry_tests)
  add_custom_target(build_tests DEPENDS strawberry_tests)
  add_custom_target(run_strawberry_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS strawberry_tests
  )
endif()

optional_component_summary_show()
