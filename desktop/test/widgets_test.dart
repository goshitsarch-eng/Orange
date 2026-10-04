import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:orange/bridge/api/models.dart';
import 'package:orange/ui/dialogs.dart';
import 'package:orange/ui/theme.dart';
import 'package:orange/ui/track_list.dart';

void main() {
  testWidgets('track rows remain usable at narrow widths, with named actions', (
    tester,
  ) async {
    tester.view.physicalSize = const Size(600, 400);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    var queued = false;
    await tester.pumpWidget(
      MaterialApp(
        theme: orangeTheme(Brightness.dark),
        home: Scaffold(
          body: TrackList(
            tracks: const [
              TrackDto(
                url: 'file:///日本.wav',
                title: '日本 Track',
                artist: 'Artist',
                album: 'Album',
                genre: 'Jazz',
                year: 1959,
                track: 1,
                duration: 180,
                rating: 0,
              ),
            ],
            play: (_) {},
            contextMenu: (_) {},
            enqueue: (_) {
              queued = true;
            },
          ),
        ),
      ),
    );
    expect(tester.takeException(), isNull);
    await tester.drag(
      find.byType(SingleChildScrollView),
      const Offset(-400, 0),
    );
    await tester.pumpAndSettle();
    await tester.tap(find.byTooltip('Add to queue'));
    await tester.pump();
    expect(queued, isTrue);
    expect(find.byTooltip('Track actions'), findsOneWidget);
  });
  testWidgets(
    'playlist dialog rejects empty names and returns a Unicode name',
    (tester) async {
      String? result;
      await tester.pumpWidget(
        MaterialApp(
          home: Builder(
            builder: (context) => TextButton(
              onPressed: () async {
                result = await showDialog<String>(
                  context: context,
                  builder: (_) => const NameDialog(),
                );
              },
              child: const Text('Open'),
            ),
          ),
        ),
      );
      await tester.tap(find.text('Open'));
      await tester.pumpAndSettle();
      await tester.tap(find.text('Save'));
      await tester.pump();
      expect(find.text('Enter a playlist name.'), findsOneWidget);
      await tester.enterText(find.byType(TextFormField), 'Música 日本');
      await tester.tap(find.text('Save'));
      await tester.pumpAndSettle();
      expect(result, 'Música 日本');
    },
  );
}
