import 'dart:io';

import 'package:flutter_rust_bridge/flutter_rust_bridge_for_generated.dart';
import 'package:path/path.dart' as p;

import '../bridge/api/models.dart';
import '../bridge/api/session.dart';
import '../bridge/frb_generated.dart';

/// Only explicit developer overrides or the executable's own bundle are loaded.
Future<Session> connectBackend(List<String> launchUris) async {
  final override = Platform.environment['ORANGE_RUST_LIBRARY'];
  final executable = p.dirname(Platform.resolvedExecutable);
  final library =
      override ??
      switch (Platform.operatingSystem) {
        'windows' => p.join(executable, 'orange_bridge.dll'),
        'macos' => p.normalize(
          p.join(executable, '..', 'Frameworks', 'liborange_bridge.dylib'),
        ),
        _ => p.join(executable, 'lib', 'liborange_bridge.so'),
      };
  if (!p.isAbsolute(library) || !File(library).existsSync()) {
    throw StateError(
      'Orange’s native library is missing from its application bundle. Rebuild or reinstall Orange.',
    );
  }
  await RustLib.init(externalLibrary: ExternalLibrary.open(library));
  return openSession(
    profile: Platform.environment['ORANGE_PROFILE_DIR'],
    launchUris: launchUris,
  );
}

String failureMessage(Object failure) => switch (failure) {
  BridgeFailure(:final message) => message,
  _ => failure.toString(),
};
