import 'dart:io';

import 'package:file_selector/file_selector.dart';
import 'package:flutter/widgets.dart';
import 'package:url_launcher/url_launcher.dart';
import 'package:window_manager/window_manager.dart';

import '../bridge/api/session.dart';

class DesktopPlatform {
  static bool get isMacOS => Platform.isMacOS;
  bool get usesCommandKey => Platform.isMacOS;
  Future<List<String>> openMusic() async => (await openFiles(
    acceptedTypeGroups: [
      XTypeGroup(label: 'Audio', extensions: await supportedAudioExtensions()),
    ],
  )).map((file) => file.path).toList();
  Future<String?> folder() => getDirectoryPath();
  Future<String?> openPlaylist() async => (await openFile(
    acceptedTypeGroups: const [
      XTypeGroup(
        label: 'Playlists',
        extensions: ['m3u', 'm3u8', 'pls', 'xspf'],
      ),
    ],
  ))?.path;
  Future<String?> save(String name) async =>
      (await getSaveLocation(suggestedName: name))?.path;
  Future<void> openProject() async {
    if (!await launchUrl(
      Uri.parse('https://github.com/goshitsarch-eng/Orange'),
      mode: LaunchMode.externalApplication,
    )) {
      throw StateError('The default browser could not open the project page.');
    }
  }

  Future<void> openFolder(Uri uri) async {
    if (!await launchUrl(uri, mode: LaunchMode.externalApplication)) {
      throw StateError('The file manager could not open this folder.');
    }
  }

  Future<void> initialize() async {
    await windowManager.ensureInitialized();
    await windowManager.setPreventClose(true);
    await windowManager.setMinimumSize(const Size(600, 400));
    await windowManager.setTitle('Orange Music Player');
  }
}
