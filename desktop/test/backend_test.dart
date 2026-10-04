import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_rust_bridge/flutter_rust_bridge_for_generated.dart';
import 'package:orange/bridge/api/models.dart';
import 'package:orange/bridge/api/session.dart';
import 'package:orange/bridge/frb_generated.dart';

void main() {
  setUpAll(() async {
    final library = Platform.environment['ORANGE_RUST_LIBRARY'];
    if (library == null) {
      throw StateError(
        'Build orange-bridge with --features native and set ORANGE_RUST_LIBRARY to its absolute path.',
      );
    }
    await RustLib.init(externalLibrary: ExternalLibrary.open(library));
  });
  test('real bridge scans Unicode audio, queues, saves playlists and persists preferences', () async {
    final root = await Directory.systemTemp.createTemp('orange-native-');
    final music = await Directory(
      '${root.path}${Platform.pathSeparator}Música 日本',
    ).create();
    await File('${music.path}${Platform.pathSeparator}01 First.wav')
        .writeAsBytes(wav());
    await File('${music.path}${Platform.pathSeparator}02 日本.wav')
        .writeAsBytes(wav());
    final session = await openSession(profile: root.path, launchUris: []);
    await waitFor(() async => (await session.playback()).libraryRevision > 0);
    await session.dispatch(command: Command.addFolder(path: music.path));
    final query = LibraryQuery(
      text: '',
      genre: '',
      artist: '',
      album: '',
      smart: SmartView.all,
      offset: 0,
      limit: 200,
    );
    await waitFor(
      () async => (await session.queryLibrary(query: query)).total == 2,
    );
    final songs = await session.queryLibrary(query: query);
    expect(
      songs.tracks.map((s) => s.title).any((s) => s.contains('日本')),
      isTrue,
    );
    await session.dispatch(
      command: Command.enqueue(urls: songs.tracks.map((s) => s.url).toList()),
    );
    await waitFor(() async => (await session.queue()).length == 2);
    await session.dispatch(
      command: const Command.savePlaylist(name: 'Música 日本'),
    );
    await waitFor(
      () async =>
          (await session.catalog()).playlists.any((p) => p.name == 'Música 日本'),
    );
    await session.dispatch(command: const Command.setVolume(volume: 37));
    await session.dispatch(command: const Command.setTheme(theme: 'dark'));
    await session.close();
    session.dispose();
    final reopened = await openSession(profile: root.path, launchUris: []);
    await waitFor(() async => (await reopened.playback()).libraryRevision > 0);
    final state = await reopened.playback();
    expect(state.preferences.volume, 37);
    expect(state.preferences.theme, 'dark');
    expect((await reopened.queue()).length, 2);
    await reopened.close();
    reopened.dispose();
    await root.delete(recursive: true);
  });
  test(
    'conversion capabilities preserve all eight targets and create real audio',
    () async {
      final root = await Directory.systemTemp.createTemp('orange-convert-');
      final source = File('${root.path}/Música 日本.wav');
      await source.writeAsBytes(wav());
      final targets = await conversionTargets();
      expect(targets.map((target) => target.name).toSet(), {
        'MP3',
        'AAC',
        'FLAC',
        'Ogg Vorbis',
        'Opus',
        'Speex',
        'WavPack',
        'ASF',
      });
      expect(
        targets.any((target) => target.name == 'FLAC' && target.available),
        isTrue,
      );
      final session = await openSession(profile: root.path, launchUris: []);
      await waitFor(() async => (await session.playback()).libraryRevision > 0);
      try {
        for (final target in targets.where((target) => target.available)) {
          final output = File('${root.path}/Converted.${target.extension_}');
          await session.dispatch(
            command: Command.convertAudio(
              source: source.path,
              format: target.name,
              destination: output.path,
            ),
          );
          await waitFor(
            () async =>
                await output.exists() && !(await session.playback()).busy,
          );
          expect((await session.playback()).error, isNull, reason: target.name);
          expect(await output.length(), greaterThan(100), reason: target.name);
        }
      } finally {
        await session.close();
        session.dispose();
        await root.delete(recursive: true);
      }
    },
  );
  test('typed validation errors, concurrent calls, bounded pages and shutdown cross the real bridge', () async {
    final root = await Directory.systemTemp.createTemp('orange-errors-');
    final session = await openSession(profile: root.path, launchUris: []);
    await waitFor(() async => (await session.playback()).libraryRevision > 0);
    await expectLater(
      session.dispatch(command: const Command.setVolume(volume: 101)),
      throwsA(
        isA<BridgeFailure>().having(
          (e) => e.kind,
          'kind',
          FailureKind.validation,
        ),
      ),
    );
    await expectLater(
      session.dispatch(
        command: const Command.addStation(
          name: 'Bad',
          url: 'https://user:secret@example.org/a',
        ),
      ),
      throwsA(isA<BridgeFailure>()),
    );
    final states = await Future.wait(
      List.generate(12, (_) => session.playback()),
    );
    expect(states.map((s) => s.libraryRevision).toSet().length, 1);
    await session.close();
    await expectLater(
      session.playback(),
      throwsA(
        isA<BridgeFailure>().having((e) => e.kind, 'kind', FailureKind.closed),
      ),
    );
    session.dispose();
    await root.delete(recursive: true);
  });
}

Future<void> waitFor(Future<bool> Function() condition) async {
  for (var i = 0; i < 150; i++) {
    if (await condition()) {
      return;
    }
    await Future<void>.delayed(const Duration(milliseconds: 20));
  }
  fail('Backend operation did not complete.');
}

Uint8List wav() {
  const size = 8000 * 2 * 3;
  final bytes = Uint8List(44 + size);
  final b = ByteData.sublistView(bytes);
  void label(int at, String value) {
    bytes.setRange(at, at + value.length, value.codeUnits);
  }

  label(0, 'RIFF');
  b.setUint32(4, 36 + size, Endian.little);
  label(8, 'WAVE');
  label(12, 'fmt ');
  b.setUint32(16, 16, Endian.little);
  b.setUint16(20, 1, Endian.little);
  b.setUint16(22, 1, Endian.little);
  b.setUint32(24, 8000, Endian.little);
  b.setUint32(28, 16000, Endian.little);
  b.setUint16(32, 2, Endian.little);
  b.setUint16(34, 16, Endian.little);
  label(36, 'data');
  b.setUint32(40, size, Endian.little);
  return bytes;
}
