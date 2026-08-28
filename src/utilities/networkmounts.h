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

#ifndef NETWORKMOUNTS_H
#define NETWORKMOUNTS_H

#include <QList>
#include <QString>

namespace Utilities {

// A network share reachable through the local filesystem.
struct NetworkMount {
  QString name;
  QString path;
};

// Network shares the user has mounted: GVfs FUSE mounts (shares opened in GNOME Files), kio-fuse mounts (shares opened in Dolphin), and network filesystems mounted the traditional way (cifs, nfs, sshfs, davfs).
QList<NetworkMount> NetworkMounts();

}  // namespace Utilities

#endif  // NETWORKMOUNTS_H
