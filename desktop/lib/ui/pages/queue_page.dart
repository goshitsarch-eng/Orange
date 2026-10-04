import 'dart:async';

import 'package:flutter/material.dart';

import '../../bridge/api/models.dart';
import '../../commands/desktop_commands.dart';
import '../../state/models.dart';
import '../track_list.dart';

class QueuePage extends StatelessWidget {
  const QueuePage({
    super.key,
    required this.session,
    required this.commands,
    required this.trackActions,
  });
  final OrangeSession session;
  final DesktopCommands commands;
  final void Function(TrackDto, bool) trackActions;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: Listenable.merge([session.queue, session.playback]),
    builder: (context, _) {
      final s = session.playback.value;
      return Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Text('Play Queue', style: Theme.of(context).textTheme.headlineSmall),
          const SizedBox(height: 10),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            crossAxisAlignment: WrapCrossAlignment.center,
            children: [
              FilledButton(
                onPressed: session.queue.tracks.isEmpty
                    ? null
                    : () => commands.invoke(DesktopAction.savePlaylist),
                child: const Text('Save Playlist'),
              ),
              TextButton(
                onPressed: session.queue.tracks.isEmpty
                    ? null
                    : () => unawaited(
                        session.execute(const Command.clearQueue()),
                      ),
                child: const Text('Clear Queue'),
              ),
              TextButton(
                onPressed: s?.canUndo == true
                    ? () => commands.invoke(DesktopAction.undo)
                    : null,
                child: const Text('Undo'),
              ),
              TextButton(
                onPressed: s?.canRedo == true
                    ? () => commands.invoke(DesktopAction.redo)
                    : null,
                child: const Text('Redo'),
              ),
              SizedBox(
                width: 150,
                child: DropdownButtonFormField<int>(
                  isExpanded: true,
                  key: ValueKey('repeat-${s?.preferences.repeat}'),
                  initialValue: s?.preferences.repeat ?? 0,
                  decoration: const InputDecoration(labelText: 'Repeat'),
                  items: [
                    for (final (i, name) in [
                      (0, 'Off'),
                      (1, 'Queue'),
                      (2, 'Track'),
                      (3, 'Album'),
                    ])
                      DropdownMenuItem(
                        value: i,
                        child: Text(name, overflow: TextOverflow.ellipsis),
                      ),
                  ],
                  onChanged: (v) {
                    if (v != null) {
                      unawaited(session.execute(Command.setRepeat(mode: v)));
                    }
                  },
                ),
              ),
              SizedBox(
                width: 150,
                child: DropdownButtonFormField<int>(
                  isExpanded: true,
                  key: ValueKey('shuffle-${s?.preferences.shuffle}'),
                  initialValue: s?.preferences.shuffle ?? 0,
                  decoration: const InputDecoration(labelText: 'Shuffle'),
                  items: [
                    for (final (i, name) in [
                      (0, 'Off'),
                      (1, 'Tracks'),
                      (2, 'Inside album'),
                    ])
                      DropdownMenuItem(
                        value: i,
                        child: Text(name, overflow: TextOverflow.ellipsis),
                      ),
                  ],
                  onChanged: (v) {
                    if (v != null) {
                      unawaited(session.execute(Command.setShuffle(mode: v)));
                    }
                  },
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Expanded(
            child: TrackList(
              tracks: session.queue.tracks,
              cursor: s?.cursor,
              play: (i) =>
                  unawaited(session.execute(Command.playQueue(index: i))),
              remove: (i) =>
                  unawaited(session.execute(Command.removeQueue(index: i))),
              contextMenu: (i) => trackActions(session.queue.tracks[i], false),
            ),
          ),
        ],
      );
    },
  );
}
