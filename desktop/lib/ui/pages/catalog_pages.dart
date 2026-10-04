import 'dart:async';

import 'package:flutter/material.dart';
import 'package:path/path.dart' as p;

import '../../bridge/api/models.dart';
import '../../bridge/api/session.dart';
import '../../commands/desktop_commands.dart';
import '../../platform/desktop_platform.dart';
import '../../state/models.dart';
import '../dialogs.dart';

class PlaylistsPage extends StatelessWidget {
  const PlaylistsPage({
    super.key,
    required this.session,
    required this.commands,
  });
  final OrangeSession session;
  final DesktopCommands commands;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: session.catalog,
    builder: (context, _) => Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Text('Playlists', style: Theme.of(context).textTheme.headlineSmall),
        const SizedBox(height: 12),
        Wrap(
          spacing: 8,
          children: [
            FilledButton(
              onPressed: () => commands.invoke(DesktopAction.savePlaylist),
              child: const Text('Save current queue'),
            ),
            TextButton(
              onPressed: () => commands.invoke(DesktopAction.importPlaylist),
              child: const Text('Import Playlist…'),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Expanded(
          child: ListView(
            children: [
              for (final playlist
                  in session.catalog.value?.playlists ?? <NamedItem>[])
                ListTile(
                  title: Text(playlist.name),
                  leading: const Icon(Icons.playlist_play),
                  trailing: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      TextButton(
                        onPressed: () {
                          unawaited(
                            session.execute(
                              Command.loadPlaylist(id: playlist.id),
                            ),
                          );
                          session.page.value = AppPage.queue;
                        },
                        child: const Text('Load'),
                      ),
                      IconButton(
                        tooltip: 'Delete playlist',
                        icon: const Icon(Icons.delete_outline),
                        onPressed: () async {
                          if (await confirmRemoval(
                            context,
                            'Delete playlist?',
                            'Remove “${playlist.name}”? Music files stay on disk.',
                          )) {
                            await session.execute(
                              Command.deletePlaylist(id: playlist.id),
                            );
                          }
                        },
                      ),
                    ],
                  ),
                ),
            ],
          ),
        ),
      ],
    ),
  );
}

class RadioPage extends StatefulWidget {
  const RadioPage({super.key, required this.session});
  final OrangeSession session;
  @override
  State<RadioPage> createState() => _RadioPageState();
}

class _RadioPageState extends State<RadioPage> {
  final _name = TextEditingController(),
      _url = TextEditingController(),
      _search = TextEditingController();
  List<StationDto> _presets = [];
  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  Future<void> _load() async {
    try {
      final presets = await radioPresets();
      if (mounted) {
        setState(() => _presets = presets);
      }
    } catch (error) {
      widget.session.report(error);
    }
  }

  @override
  void dispose() {
    _name.dispose();
    _url.dispose();
    _search.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: widget.session.catalog,
    builder: (context, _) => ListView(
      children: [
        Text('Radio', style: Theme.of(context).textTheme.headlineSmall),
        const SizedBox(height: 12),
        TextField(
          controller: _search,
          decoration: const InputDecoration(labelText: 'Search Radio Browser'),
        ),
        Align(
          alignment: Alignment.centerLeft,
          child: TextButton(
            onPressed: () => unawaited(
              widget.session.execute(Command.searchRadio(text: _search.text)),
            ),
            child: const Text('Search stations'),
          ),
        ),
        for (final station
            in widget.session.catalog.value?.radioResults ?? <StationDto>[])
          _station(station),
        const Divider(),
        Text('Saved stations', style: Theme.of(context).textTheme.titleMedium),
        for (final (index, station)
            in (widget.session.catalog.value?.stations ?? <StationDto>[])
                .indexed)
          _station(station, index: index),
        const SizedBox(height: 12),
        TextField(
          controller: _name,
          decoration: const InputDecoration(labelText: 'Station name'),
        ),
        const SizedBox(height: 10),
        TextField(
          controller: _url,
          decoration: const InputDecoration(
            labelText: 'Stream URL',
            hintText: 'https://…',
          ),
        ),
        Align(
          alignment: Alignment.centerLeft,
          child: FilledButton(
            onPressed: () => unawaited(
              widget.session.execute(
                Command.addStation(name: _name.text, url: _url.text),
              ),
            ),
            child: const Text('Save station'),
          ),
        ),
        const Divider(),
        Text(
          'Radio Paradise and SomaFM',
          style: Theme.of(context).textTheme.titleMedium,
        ),
        for (final station in _presets) _station(station),
      ],
    ),
  );
  Widget _station(StationDto station, {int? index}) => ListTile(
    title: Text(station.name),
    subtitle: Text(station.url, maxLines: 1, overflow: TextOverflow.ellipsis),
    leading: const Icon(Icons.radio),
    trailing: Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        IconButton(
          tooltip: 'Play station',
          icon: const Icon(Icons.play_arrow),
          onPressed: () => unawaited(
            widget.session.execute(Command.openUris(uris: [station.url])),
          ),
        ),
        if (index != null)
          IconButton(
            tooltip: 'Remove saved station',
            icon: const Icon(Icons.delete_outline),
            onPressed: () => unawaited(
              widget.session.execute(Command.removeStation(index: index)),
            ),
          ),
      ],
    ),
  );
}

class FilesPage extends StatelessWidget {
  const FilesPage({super.key, required this.session, required this.platform});
  final OrangeSession session;
  final DesktopPlatform platform;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: session.catalog,
    builder: (context, _) {
      final data = session.catalog.value;
      final path = data?.filesPath;
      return Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Text('Files', style: Theme.of(context).textTheme.headlineSmall),
          const SizedBox(height: 12),
          Wrap(
            spacing: 8,
            children: [
              FilledButton.icon(
                icon: const Icon(Icons.folder_open),
                label: const Text('Choose Folder…'),
                onPressed: () async {
                  try {
                    final path = await platform.folder();
                    if (path != null) {
                      await session.execute(Command.browseFolder(path: path));
                    }
                  } catch (error) {
                    session.report(error);
                  }
                },
              ),
              TextButton(
                onPressed: path != null
                    ? () => unawaited(
                        session.execute(
                          Command.browseFolder(path: p.dirname(path)),
                        ),
                      )
                    : null,
                child: const Text('Up'),
              ),
              TextButton(
                onPressed: path != null
                    ? () => unawaited(
                        session.execute(Command.playFolder(path: path)),
                      )
                    : null,
                child: const Text('Play Folder'),
              ),
            ],
          ),
          const SizedBox(height: 10),
          SelectableText(
            path ?? 'Choose a folder to browse music without adding it to your collection.',
          ),
          const SizedBox(height: 12),
          Expanded(
            child: ListView.builder(
              itemCount: data?.files.length ?? 0,
              itemBuilder: (context, index) {
                final file = data!.files[index];
                return ListTile(
                  leading: Icon(
                    file.directory ? Icons.folder : Icons.audio_file,
                  ),
                  title: Text(file.name),
                  onTap: () => unawaited(
                    session.execute(
                      file.directory
                          ? Command.browseFolder(path: file.path)
                          : Command.openFiles(paths: [file.path]),
                    ),
                  ),
                );
              },
            ),
          ),
        ],
      );
    },
  );
}

class DevicesPage extends StatelessWidget {
  const DevicesPage({super.key, required this.session, required this.platform});
  final OrangeSession session;
  final DesktopPlatform platform;
  @override
  Widget build(BuildContext context) => ListView(
    children: [
      Text('Devices', style: Theme.of(context).textTheme.headlineSmall),
      const SizedBox(height: 12),
      const Text(
        'Copy the current queue to a selected folder or a mounted music device. Existing files are preserved.',
      ),
      const SizedBox(height: 16),
      Align(
        alignment: Alignment.centerLeft,
        child: FilledButton.icon(
          icon: const Icon(Icons.copy),
          label: const Text('Choose Destination and Copy Queue…'),
          onPressed: () async {
            try {
              final destination = await platform.folder();
              if (destination != null) {
                await session.execute(
                  Command.copyQueue(destination: destination),
                );
              }
            } catch (error) {
              session.report(error);
            }
          },
        ),
      ),
      const SizedBox(height: 16),
      const Text(
        'For a phone or music player, mount its storage first and select the music folder.',
      ),
    ],
  );
}
