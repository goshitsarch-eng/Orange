#include "ui/dialogs.h"

#include "collection/groupbydialog.h"
#include "collection/savedgroupingmanager.h"
#include "covermanager/albumcoverexportdialog.h"
#include "covermanager/albumcovermanager.h"
#include "covermanager/albumcoversearcher.h"
#include "covermanager/coverfromurldialog.h"
#include "device/copytodevicedialog.h"
#include "dialogs/addstreamdialog.h"
#include "dialogs/console.h"
#include "dialogs/deleteconfirmationdialog.h"
#include "dialogs/edittagdialog.h"
#include "dialogs/errordialog.h"
#include "dialogs/saveplaylistsdialog.h"
#include "dialogs/shortcutsdialog.h"
#include "dialogs/trackselectiondialog.h"
#include "dialogs/userpassdialog.h"
#include "equalizer/equalizerdialog.h"
#include "globalshortcuts/globalshortcutgrabber.h"
#include "organize/organizedialog.h"
#include "playlist/playlistcolumnsdialog.h"
#include "smartplaylists/smartplaylistwizard.h"
#include "transcoder/transcodedialog.h"

void Dialogs::AddStream(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback) {
  AddStreamDialog::Show(parent, callback);
}

void Dialogs::CoverManager(GtkWindow *parent, Application *app) { AlbumCoverManager::Show(parent, app); }

void Dialogs::CoverFromUrl(GtkWindow *parent, Application *app) { CoverFromUrlDialog::Show(parent, app); }

void Dialogs::CoverSearch(GtkWindow *parent, Application *app) { AlbumCoverSearcher::Show(parent, app); }

void Dialogs::CoverExport(GtkWindow *parent, Application *app) { AlbumCoverExportDialog::Show(parent, app); }

void Dialogs::Equalizer(GtkWindow *parent, class Equalizer *equalizer) { EqualizerDialog::Show(parent, equalizer); }

void Dialogs::Transcode(GtkWindow *parent, Application *app) { TranscodeDialog::Show(parent, app); }

void Dialogs::Organize(GtkWindow *parent, Application *app) { OrganizeDialog::Show(parent, app); }

void Dialogs::TagFetcher(GtkWindow *parent, Application *app) { TrackSelectionDialog::Show(parent, app); }

void Dialogs::EditTag(GtkWindow *parent, Application *app, const SongList &songs) { EditTagDialog::Show(parent, app, songs); }

void Dialogs::Shortcuts(GtkWindow *parent) { ShortcutsDialog::Show(parent); }

void Dialogs::GrabShortcut(GtkWindow *parent, const std::function<void(const std::string &)> &callback) {
  GlobalShortcutGrabber::Show(parent, callback);
}

void Dialogs::Login(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback) {
  UserPassDialog::Show(parent, service, callback);
}

void Dialogs::SmartPlaylistWizard(GtkWindow *parent, Application *app) { SmartPlaylistWizard::Show(parent, app); }

void Dialogs::GroupBy(GtkWindow *parent, const CollectionGrouping::Grouping &current,
                      const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  GroupByDialog::Show(parent, current, callback);
}

void Dialogs::ManageSavedGroupings(GtkWindow *parent, const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  SavedGroupingManager::Show(parent, callback);
}

void Dialogs::PlaylistColumns(GtkWindow *parent, const std::function<void()> &callback) {
  PlaylistColumnsDialog::Show(parent, callback);
}

void Dialogs::DeleteFiles(GtkWindow *parent, Application *app) { DeleteConfirmationDialog::Show(parent, app); }

void Dialogs::CopyToDevice(GtkWindow *parent, Application *app) { CopyToDeviceDialog::Show(parent, app); }

void Dialogs::SaveAllPlaylists(GtkWindow *parent, Application *app) { SavePlaylistsDialog::Show(parent, app); }

void Dialogs::Console(GtkWindow *parent) { Console::Show(parent); }

void Dialogs::Error(GtkWindow *parent, const std::string &message) { ErrorDialog::Show(parent, message); }
