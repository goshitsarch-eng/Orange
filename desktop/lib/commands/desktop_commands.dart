import 'dart:async';

import 'package:flutter/widgets.dart';

import '../bridge/api/models.dart';
import '../platform/desktop_platform.dart';
import '../state/models.dart';

enum DesktopAction {
  open,
  folder,
  importPlaylist,
  exportQueue,
  rescan,
  playPause,
  stop,
  next,
  previous,
  undo,
  redo,
  preferences,
  about,
  lyrics,
  savePlaylist,
  quit,
}

class DesktopIntent extends Intent {
  const DesktopIntent(this.command);
  final DesktopAction command;
}

class DesktopCommands {
  DesktopCommands(this.session, this.platform, this.dialog, this.quit);
  final OrangeSession session;
  final DesktopPlatform platform;
  final void Function(DesktopAction) dialog;
  final Future<void> Function() quit;
  Future<void> run(DesktopAction action) async {
    try {
      switch (action) {
        case DesktopAction.open:
          final paths = await platform.openMusic();
          if (paths.isNotEmpty) {
            await session.execute(Command.openFiles(paths: paths));
          }
        case DesktopAction.folder:
          final path = await platform.folder();
          if (path != null) {
            await session.execute(Command.addFolder(path: path));
          }
        case DesktopAction.importPlaylist:
          final path = await platform.openPlaylist();
          if (path != null) {
            await session.execute(Command.importPlaylist(path: path));
            session.page.value = AppPage.queue;
          }
        case DesktopAction.exportQueue:
          final path = await platform.save('Orange Queue.m3u8');
          if (path != null) {
            await session.execute(Command.exportPlaylist(path: path));
          }
        case DesktopAction.rescan:
          await session.execute(const Command.rescan());
        case DesktopAction.playPause:
          await session.execute(const Command.playPause());
        case DesktopAction.stop:
          await session.execute(const Command.stop());
        case DesktopAction.next:
          await session.execute(const Command.next());
        case DesktopAction.previous:
          await session.execute(const Command.previous());
        case DesktopAction.undo:
          await session.execute(const Command.undoQueue());
        case DesktopAction.redo:
          await session.execute(const Command.redoQueue());
        case DesktopAction.preferences:
          session.page.value = AppPage.settings;
        case DesktopAction.quit:
          await quit();
        case DesktopAction.about:
        case DesktopAction.lyrics:
        case DesktopAction.savePlaylist:
          dialog(action);
      }
    } catch (error) {
      session.report(error);
    }
  }

  void invoke(DesktopAction action) {
    unawaited(run(action));
  }
}
