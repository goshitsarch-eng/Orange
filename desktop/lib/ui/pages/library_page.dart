import 'dart:async';

import 'package:flutter/material.dart';

import '../../bridge/api/models.dart';
import '../../commands/desktop_commands.dart';
import '../../state/models.dart';
import '../track_list.dart';

class LibraryPageView extends StatelessWidget {
  const LibraryPageView({
    super.key,
    required this.session,
    required this.commands,
    required this.trackActions,
    this.compact = false,
  });
  final bool compact;
  final OrangeSession session;
  final DesktopCommands commands;
  final void Function(TrackDto, bool) trackActions;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: session.library,
    builder: (context, _) {
      final model = session.library;
      final page = model.value;
      Widget filter(
        String label,
        String selected,
        List<String> values,
        ValueChanged<String> changed,
      ) => SizedBox(
        width: 190,
        child: DropdownButtonFormField<String>(
          key: ValueKey('$label-$selected-${values.length}'),
          initialValue: values.contains(selected) ? selected : '',
          isExpanded: true,
          decoration: InputDecoration(labelText: label),
          items: [
            DropdownMenuItem(
              value: '',
              child: Text('All ${label.toLowerCase()}s'),
            ),
            for (final value in values)
              DropdownMenuItem(
                value: value,
                child: Text(value, overflow: TextOverflow.ellipsis),
              ),
          ],
          onChanged: (v) {
            if (v != null) {
              changed(v);
            }
          },
        ),
      );
      List<Widget> filters() => [
        filter(
          'Genre',
          model.genre,
          model.value?.genres ?? [],
          (v) => model.select(newGenre: v),
        ),
        filter(
          'Artist',
          model.artist,
          model.value?.artists ?? [],
          (v) => model.select(newArtist: v),
        ),
        filter(
          'Album',
          model.album,
          model.value?.albums ?? [],
          (v) => model.select(newAlbum: v),
        ),
      ];
      void showFilters() {
        unawaited(
          showDialog<void>(
            context: context,
            builder: (context) => AlertDialog(
              title: const Text('Filter music'),
              content: ListenableBuilder(
                listenable: model,
                builder: (context, _) => SizedBox(
                  width: 220,
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      for (final control in filters())
                        Padding(
                          padding: const EdgeInsets.only(bottom: 16),
                          child: control,
                        ),
                    ],
                  ),
                ),
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(context),
                  child: const Text('Done'),
                ),
              ],
            ),
          ),
        );
      }

      return Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          if (compact)
            Row(
              children: [
                Expanded(
                  child: Text(
                    'Music',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                ),
                IconButton(
                  tooltip: 'Add Music Folder',
                  icon: const Icon(Icons.create_new_folder),
                  onPressed: () => commands.invoke(DesktopAction.folder),
                ),
                IconButton(
                  tooltip: 'Open Music',
                  icon: const Icon(Icons.audio_file),
                  onPressed: () => commands.invoke(DesktopAction.open),
                ),
                IconButton(
                  tooltip: 'Rescan',
                  icon: const Icon(Icons.refresh),
                  onPressed: () => commands.invoke(DesktopAction.rescan),
                ),
                IconButton(
                  tooltip: 'Filter music',
                  icon: const Icon(Icons.filter_list),
                  onPressed: showFilters,
                ),
              ],
            )
          else
            Wrap(
              spacing: 8,
              runSpacing: 8,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                Text('Music', style: Theme.of(context).textTheme.headlineSmall),
                FilledButton.icon(
                  onPressed: () => commands.invoke(DesktopAction.folder),
                  icon: const Icon(Icons.create_new_folder),
                  label: const Text('Add Music Folder'),
                ),
                TextButton(
                  onPressed: () => commands.invoke(DesktopAction.open),
                  child: const Text('Open Music…'),
                ),
                TextButton(
                  onPressed: () => commands.invoke(DesktopAction.rescan),
                  child: const Text('Rescan'),
                ),
                if (model.smart != SmartView.all)
                  TextButton(
                    onPressed: () => model.select(newSmart: SmartView.all),
                    child: const Text('All music'),
                  ),
              ],
            ),
          SizedBox(height: compact ? 4 : 12),
          TextFormField(
            key: const ValueKey('music-search'),
            initialValue: model.text,
            decoration: const InputDecoration(
              labelText: 'Search music',
              hintText: 'Title, artist, album or artist:…',
              prefixIcon: Icon(Icons.search),
            ),
            onChanged: model.search,
          ),
          if (!compact) ...[
            const SizedBox(height: 10),
            Wrap(spacing: 8, runSpacing: 8, children: filters()),
          ],
          SizedBox(height: compact ? 4 : 12),
          Expanded(
            child: page == null
                ? const Center(child: CircularProgressIndicator())
                : TrackList(
                    tracks: page.tracks,
                    play: (i) => unawaited(
                      session.execute(
                        Command.playLibrary(
                          query: model.query,
                          index: model.offset + i,
                        ),
                      ),
                    ),
                    enqueue: (i) => unawaited(
                      session.execute(
                        Command.enqueue(urls: [page.tracks[i].url]),
                      ),
                    ),
                    contextMenu: (i) => trackActions(page.tracks[i], true),
                  ),
          ),
          if ((page?.total ?? 0) > 200)
            Row(
              mainAxisAlignment: MainAxisAlignment.end,
              children: [
                Text(
                  '${model.offset + 1}–${model.offset + (page?.tracks.length ?? 0)} of ${page?.total ?? 0}',
                ),
                IconButton(
                  tooltip: 'Previous page',
                  onPressed: model.offset > 0
                      ? () => model.page((model.offset - 200).clamp(0, 1 << 30))
                      : null,
                  icon: const Icon(Icons.chevron_left),
                ),
                IconButton(
                  tooltip: 'Next page',
                  onPressed: model.offset + 200 < (page?.total ?? 0)
                      ? () => model.page(model.offset + 200)
                      : null,
                  icon: const Icon(Icons.chevron_right),
                ),
              ],
            ),
        ],
      );
    },
  );
}
