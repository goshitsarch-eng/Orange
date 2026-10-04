import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:orange/bridge/api/models.dart';
import 'package:orange/main.dart' as app;
import 'package:orange/ui/chrome.dart';
import 'package:window_manager/window_manager.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();
  testWidgets(
    'native Flutter controls reach real Rust services and survive theme/resize changes',
    (tester) async {
      await app.main([]);
      await wait(tester, () => find.text('Track 1').evaluate().isNotEmpty);
      final session = tester
          .widget<app.OrangeDesktop>(find.byType(app.OrangeDesktop))
          .session;
      await tester.enterText(
        find.byKey(const ValueKey('music-search')),
        'no-such-track',
      );
      await wait(tester, () => session.library.value?.total == 0);
      await tester.enterText(find.byKey(const ValueKey('music-search')), '');
      await wait(tester, () => session.library.value?.total == 2);
      await tester.tap(find.byTooltip('Add to queue').first);
      await wait(tester, () => session.queue.tracks.length == 1);
      await tester.tap(find.widgetWithText(ListTile, 'Play Queue'));
      await tester.pumpAndSettle();
      await tester.tap(find.byTooltip('Play track').first);
      await wait(
        tester,
        () => session.playback.value?.state == PlaybackState.playing,
      );
      await tester.tap(find.byTooltip('Pause'));
      await wait(
        tester,
        () => session.playback.value?.state == PlaybackState.paused,
      );
      final seek = find.descendant(
        of: find.byWidgetPredicate(
          (widget) =>
              widget is CommitSlider && widget.label == 'Playback position',
        ),
        matching: find.byType(Slider),
      );
      await tester.tapAt(tester.getRect(seek).center);
      await wait(tester, () => (session.playback.value?.position ?? 0) > 0);
      final volume = find.descendant(
        of: find.byWidgetPredicate(
          (widget) => widget is CommitSlider && widget.label == 'Volume',
        ),
        matching: find.byType(Slider),
      );
      await tester.tapAt(tester.getRect(volume).center);
      await wait(
        tester,
        () => (session.preferences.value?.volume ?? 100) < 100,
      );
      await tester.tap(find.byTooltip('Stop'));
      await wait(
        tester,
        () => session.playback.value?.state == PlaybackState.stopped,
      );
      for (final mode in [
        (1, 'Queue'),
        (2, 'Track'),
        (3, 'Album'),
        (0, 'Off'),
      ]) {
        await tester.tap(find.byType(DropdownButtonFormField<int>).first);
        await tester.pumpAndSettle();
        await tester.tap(find.text(mode.$2).last);
        await wait(tester, () => session.preferences.value?.repeat == mode.$1);
      }
      await tester.tap(find.byType(DropdownButtonFormField<int>).last);
      await tester.pumpAndSettle();
      await tester.tap(find.text('Inside album').last);
      await wait(tester, () => session.preferences.value?.shuffle == 2);
      await tester.tap(find.text('Save Playlist'));
      await tester.pumpAndSettle();
      await tester.enterText(
        find.byType(TextFormField).last,
        'Música 日本 Flutter',
      );
      await tester.tap(find.text('Save'));
      await wait(
        tester,
        () =>
            session.catalog.value?.playlists.any(
              (p) => p.name == 'Música 日本 Flutter',
            ) ??
            false,
      );
      await tester.tap(find.text('Clear Queue'));
      await wait(tester, () => session.queue.tracks.isEmpty);
      await tester.tap(find.text('Undo'));
      await wait(tester, () => session.queue.tracks.length == 1);
      await tester.tap(find.widgetWithText(ListTile, 'Settings'));
      await tester.pumpAndSettle();
      final equalizer = find.descendant(
        of: find
            .byWidgetPredicate(
              (widget) =>
                  widget is CommitSlider &&
                  widget.label.startsWith('Equalizer'),
            )
            .first,
        matching: find.byType(Slider),
      );
      await tester.drag(equalizer, const Offset(60, 0));
      await wait(
        tester,
        () => (session.preferences.value?.equalizer.first ?? 0) > 0,
      );
      for (final mode in ['Dark', 'Light', 'Follow System']) {
        await tester.tap(find.byType(DropdownButtonFormField<String>).first);
        await tester.pumpAndSettle();
        await tester.tap(find.text(mode).last);
        await tester.pumpAndSettle();
        final expected = switch (mode) {
          'Dark' => 'dark',
          'Light' => 'light',
          _ => 'system',
        };
        await wait(tester, () => session.preferences.value?.theme == expected);
        await tester.pumpAndSettle();
        expect(
          tester.widget<MaterialApp>(find.byType(MaterialApp)).themeMode,
          switch (mode) {
            'Dark' => ThemeMode.dark,
            'Light' => ThemeMode.light,
            _ => ThemeMode.system,
          },
        );
        await screenshot(tester, mode.toLowerCase().replaceAll(' ', '-'));
      }
      await tester.tap(find.widgetWithText(ListTile, 'Music'));
      await tester.pumpAndSettle();
      for (final size in [
        const Size(720, 560),
        const Size(600, 400),
        const Size(1280, 800),
      ]) {
        await windowManager.setSize(size);
        await tester.pumpAndSettle();
        expect(tester.takeException(), isNull);
        await screenshot(
          tester,
          'music-${size.width.toInt()}x${size.height.toInt()}',
        );
      }
      await session.execute(const Command.setTheme(theme: 'dark'));
      await wait(tester, () => session.preferences.value?.theme == 'dark');
      await screenshot(tester, 'music-dark');
      expect(session.playback.value?.error, isNull);
      expect(session.error.value, isNull);
      await session.close();
    },
  );
}

Future<void> wait(WidgetTester tester, bool Function() condition) async {
  for (var i = 0; i < 100; i++) {
    await tester.pump(const Duration(milliseconds: 100));
    if (condition()) {
      return;
    }
  }
  fail('Native UI state did not reach the expected result.');
}

Future<void> screenshot(WidgetTester tester, String name) async {
  await tester.pumpAndSettle();
  await Future<void>.delayed(const Duration(milliseconds: 100));
  final output = Platform.environment['ORANGE_QA_SCREENSHOTS'];
  if (output == null || !Platform.isLinux) {
    return;
  }
  await Directory(output).create(recursive: true);
  final result = await Process.run('import', [
    '-window',
    'Orange Music Player',
    '$output/$name.png',
  ]);
  if (result.exitCode != 0) {
    throw StateError('Native screenshot capture failed: ${result.stderr}');
  }
}
