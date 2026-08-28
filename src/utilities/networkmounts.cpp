/*
 * Strawberry Music Player
 * Copyright 2026, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QtGlobal>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#include "networkmounts.h"

using namespace Qt::Literals::StringLiterals;

namespace {

QString RuntimeDir() {

  QString runtime_dir = qEnvironmentVariable("XDG_RUNTIME_DIR");
#ifdef Q_OS_UNIX
  if (runtime_dir.isEmpty()) {
    runtime_dir = u"/run/user/%1"_s.arg(getuid());
  }
#endif

  return runtime_dir;

}

// GVfs mount directories are named like "smb-share:server=nas.local,share=music"; turn that into "music on nas.local".
QString PrettyGVfsName(const QString &dirname) {

  const qsizetype colon = dirname.indexOf(u':');
  if (colon < 0) return dirname;

  const QString scheme = dirname.left(colon);
  QString server;
  QString share;
  const QStringList parts = dirname.mid(colon + 1).split(u',');
  for (const QString &part : parts) {
    const qsizetype eq = part.indexOf(u'=');
    if (eq < 0) continue;
    const QString key = part.left(eq);
    const QString value = part.mid(eq + 1);
    if (key == "server"_L1 || key == "host"_L1) {
      server = value;
    }
    else if (key == "share"_L1) {
      share = value;
    }
  }

  if (!server.isEmpty() && !share.isEmpty()) return share + " on "_L1 + server;
  if (!server.isEmpty()) return server + " ("_L1 + scheme + u')';

  return dirname;

}

#ifdef Q_OS_LINUX
// Mount points in /proc/self/mounts have spaces and other special characters octal-escaped.
QString UnescapeMountPath(const QString &path) {

  QString ret = path;
  ret.replace("\\040"_L1, " "_L1);
  ret.replace("\\011"_L1, "\t"_L1);
  ret.replace("\\012"_L1, "\n"_L1);
  ret.replace("\\134"_L1, "\\"_L1);

  return ret;

}
#endif

}  // namespace

QList<Utilities::NetworkMount> Utilities::NetworkMounts() {

  QList<NetworkMount> mounts;
  QSet<QString> seen_paths;

  const auto add_mount = [&mounts, &seen_paths](const QString &name, const QString &path) {
    if (seen_paths.contains(path)) return;
    seen_paths.insert(path);
    mounts << NetworkMount{ name, path };
  };

  const QString runtime_dir = RuntimeDir();
  if (!runtime_dir.isEmpty()) {

    // GVfs FUSE mounts: shares mounted through GNOME Files show up as directories under $XDG_RUNTIME_DIR/gvfs.
    const QFileInfoList gvfs_entries = QDir(runtime_dir + "/gvfs"_L1).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &gvfs_entry : gvfs_entries) {
      add_mount(PrettyGVfsName(gvfs_entry.fileName()), gvfs_entry.absoluteFilePath());
    }

    // kio-fuse mounts: shares opened through Dolphin show up under $XDG_RUNTIME_DIR/kio-fuse-*/<protocol>/<host>.
    const QFileInfoList kio_roots = QDir(runtime_dir).entryInfoList(QStringList() << u"kio-fuse-*"_s, QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &kio_root : kio_roots) {
      const QFileInfoList protocols = QDir(kio_root.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
      for (const QFileInfo &protocol : protocols) {
        const QFileInfoList hosts = QDir(protocol.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &host : hosts) {
          add_mount(host.fileName() + " ("_L1 + protocol.fileName() + u')', host.absoluteFilePath());
        }
      }
    }

  }

#ifdef Q_OS_LINUX
  // Network filesystems mounted the traditional way (fstab, mount, autofs).
  static const QStringList network_filesystems{ u"cifs"_s, u"smb3"_s, u"smbfs"_s, u"nfs"_s, u"nfs4"_s, u"fuse.sshfs"_s, u"davfs"_s, u"fuse.davfs2"_s, u"fuse.curlftpfs"_s, u"afpfs"_s };
  QFile mounts_file(u"/proc/self/mounts"_s);
  if (mounts_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    while (!mounts_file.atEnd()) {
      const QString line = QString::fromUtf8(mounts_file.readLine());
      const QStringList fields = line.split(u' ', Qt::SkipEmptyParts);
      if (fields.count() < 3) continue;
      const QString &device = fields.at(0);
      const QString mount_path = UnescapeMountPath(fields.at(1));
      const QString &filesystem = fields.at(2);
      if (!network_filesystems.contains(filesystem)) continue;
      add_mount(device, mount_path);
    }
    mounts_file.close();
  }
#endif

  return mounts;

}
