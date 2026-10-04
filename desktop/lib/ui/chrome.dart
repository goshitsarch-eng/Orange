import 'dart:async';

import 'package:flutter/material.dart';

import '../bridge/api/models.dart';
import '../commands/desktop_commands.dart';
import '../state/models.dart';
import '../platform/desktop_platform.dart';
import 'theme.dart';
import 'spectrum.dart';

class AppMenus extends StatelessWidget {
  const AppMenus({super.key, required this.commands});
  final DesktopCommands commands;
  @override
  Widget build(BuildContext context) {
    MenuItemButton item(String label, DesktopAction action) => MenuItemButton(
      onPressed: () => commands.invoke(action),
      child: Text(label),
    );
    if (DesktopPlatform.isMacOS) {
      PlatformMenuItem native(String label, DesktopAction action) =>
          PlatformMenuItem(
            label: label,
            onSelected: () => commands.invoke(action),
          );
      return PlatformMenuBar(
        menus: [
          PlatformMenu(
            label: 'Orange',
            menus: [
              native('About Orange', DesktopAction.about),
              native('Settings…', DesktopAction.preferences),
              native('Quit Orange', DesktopAction.quit),
            ],
          ),
          PlatformMenu(
            label: 'File',
            menus: [
              native('Open Music…', DesktopAction.open),
              native('Add Music Folder…', DesktopAction.folder),
              native('Import Playlist…', DesktopAction.importPlaylist),
              native('Export Queue…', DesktopAction.exportQueue),
            ],
          ),
          PlatformMenu(
            label: 'Edit',
            menus: [
              native('Undo Queue Change', DesktopAction.undo),
              native('Redo Queue Change', DesktopAction.redo),
            ],
          ),
          PlatformMenu(
            label: 'Playback',
            menus: [
              native('Play / Pause', DesktopAction.playPause),
              native('Stop', DesktopAction.stop),
              native('Next', DesktopAction.next),
              native('Previous', DesktopAction.previous),
              native('Lyrics', DesktopAction.lyrics),
            ],
          ),
        ],
        child: const SizedBox.shrink(),
      );
    }
    return MenuBar(
      children: [
        SubmenuButton(
          menuChildren: [
            item('About Orange', DesktopAction.about),
            item('Settings…', DesktopAction.preferences),
            item('Quit Orange', DesktopAction.quit),
          ],
          child: const Text('Orange'),
        ),
        SubmenuButton(
          menuChildren: [
            item('Open Music…', DesktopAction.open),
            item('Add Music Folder…', DesktopAction.folder),
            item('Import Playlist…', DesktopAction.importPlaylist),
            item('Export Queue…', DesktopAction.exportQueue),
          ],
          child: const Text('File'),
        ),
        SubmenuButton(
          menuChildren: [
            item('Undo Queue Change', DesktopAction.undo),
            item('Redo Queue Change', DesktopAction.redo),
          ],
          child: const Text('Edit'),
        ),
        SubmenuButton(
          menuChildren: [
            item('Play / Pause', DesktopAction.playPause),
            item('Stop', DesktopAction.stop),
            item('Next', DesktopAction.next),
            item('Previous', DesktopAction.previous),
            MenuItemButton(
              onPressed: () => unawaited(
                commands.session.execute(const Command.stopAfterCurrent()),
              ),
              child: const Text('Stop After Current'),
            ),
            item('Lyrics', DesktopAction.lyrics),
          ],
          child: const Text('Playback'),
        ),
      ],
    );
  }
}

class Transport extends StatelessWidget {
  const Transport({super.key, required this.session, required this.commands});
  final OrangeSession session;
  final DesktopCommands commands;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: session.playback,
    builder: (context, _) {
      final s = session.playback.value;
      final ready = session.queue.tracks.isNotEmpty;
      final playing = s?.state == PlaybackState.playing;
      final active =
          s?.state == PlaybackState.playing || s?.state == PlaybackState.paused;
      Widget controls() => Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          IconButton(
            tooltip: 'Previous track',
            onPressed: ready
                ? () => commands.invoke(DesktopAction.previous)
                : null,
            icon: const Icon(Icons.skip_previous),
          ),
          IconButton.filled(
            tooltip: playing ? 'Pause' : 'Play',
            onPressed: ready
                ? () => commands.invoke(DesktopAction.playPause)
                : null,
            icon: Icon(playing ? Icons.pause : Icons.play_arrow),
          ),
          IconButton(
            tooltip: 'Next track',
            onPressed: ready ? () => commands.invoke(DesktopAction.next) : null,
            icon: const Icon(Icons.skip_next),
          ),
          IconButton(
            tooltip: 'Stop',
            onPressed: active
                ? () => commands.invoke(DesktopAction.stop)
                : null,
            icon: const Icon(Icons.stop),
          ),
        ],
      );
      Widget title() => Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            s?.current?.title ?? 'Not playing',
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: Theme.of(context).textTheme.titleMedium,
          ),
          Text(
            s?.current == null
                ? 'Your music, your collection'
                : '${s!.current!.artist} · ${s.current!.album}',
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
          if (playing && (s?.spectrum.isNotEmpty ?? false))
            SpectrumView(decibels: s!.spectrum),
        ],
      );
      Widget seek() => Row(
        children: [
          Text(durationLabel(s?.position ?? 0)),
          Expanded(
            child: CommitSlider(
              label: 'Playback position',
              value: (s?.position ?? 0).toDouble(),
              max: (s?.duration ?? 0).toDouble().clamp(1, double.infinity),
              enabled: active && (s?.duration ?? 0) > 0,
              commit: (v) =>
                  unawaited(session.execute(Command.seek(seconds: v.round()))),
            ),
          ),
          Text(durationLabel(s?.duration ?? 0)),
        ],
      );
      Widget volume() => SizedBox(
        width: 180,
        child: Row(
          children: [
            const Icon(Icons.volume_up, size: 18),
            Expanded(
              child: CommitSlider(
                label: 'Volume',
                value: (s?.preferences.volume ?? 100).toDouble(),
                max: 100,
                commit: (v) => unawaited(
                  session.execute(Command.setVolume(volume: v.round())),
                ),
              ),
            ),
          ],
        ),
      );
      return Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
        child: LayoutBuilder(
          builder: (context, bounds) => bounds.maxWidth < 850
              ? Column(
                  children: [
                    Row(
                      children: [
                        controls(),
                        const SizedBox(width: 12),
                        Expanded(child: title()),
                      ],
                    ),
                    Row(
                      children: [
                        Expanded(child: seek()),
                        volume(),
                      ],
                    ),
                  ],
                )
              : Row(
                  children: [
                    controls(),
                    const SizedBox(width: 16),
                    Expanded(flex: 2, child: title()),
                    const SizedBox(width: 20),
                    Expanded(flex: 3, child: seek()),
                    volume(),
                  ],
                ),
        ),
      );
    },
  );
}

/// Drag state stays in Dart; only a committed operation crosses the bridge.
class CommitSlider extends StatefulWidget {
  const CommitSlider({
    super.key,
    required this.label,
    required this.value,
    required this.max,
    required this.commit,
    this.min = 0,
    this.enabled = true,
  });
  final String label;
  final double value, min, max;
  final bool enabled;
  final ValueChanged<double> commit;
  @override
  State<CommitSlider> createState() => _CommitSliderState();
}

class _CommitSliderState extends State<CommitSlider> {
  double? _drag;
  @override
  Widget build(BuildContext context) => Semantics(
    label: widget.label,
    child: Slider(
      min: widget.min,
      max: widget.max,
      value: (_drag ?? widget.value).clamp(widget.min, widget.max),
      label: '${widget.label}: ${(_drag ?? widget.value).round()}',
      onChanged: widget.enabled ? (v) => setState(() => _drag = v) : null,
      onChangeEnd: widget.enabled
          ? (v) {
              widget.commit(v);
              setState(() => _drag = null);
            }
          : null,
    ),
  );
}

class Sidebar extends StatelessWidget {
  const Sidebar({super.key, required this.session, required this.commands});
  final OrangeSession session;
  final DesktopCommands commands;
  static const labels = [
    'Music',
    'Play Queue',
    'Playlists',
    'Radio',
    'Files',
    'Devices',
    'Settings',
  ];
  static const icons = [
    Icons.library_music,
    Icons.queue_music,
    Icons.playlist_play,
    Icons.radio,
    Icons.folder,
    Icons.usb,
    Icons.settings,
  ];
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: Listenable.merge([
      session.page,
      session.catalog,
      session.library,
    ]),
    builder: (context, _) => Material(
      color: Theme.of(context).colorScheme.surfaceContainerLow,
      child: ListView(
        padding: const EdgeInsets.symmetric(vertical: 8),
        children: [
          const Padding(padding: EdgeInsets.all(12), child: Text('LIBRARY')),
          for (final page in AppPage.values)
            ListTile(
              dense: true,
              selected: session.page.value == page,
              leading: Icon(icons[page.index], size: 20),
              title: Text(labels[page.index]),
              onTap: () => session.page.value = page,
            ),
          const Padding(
            padding: EdgeInsets.all(12),
            child: Text('SMART PLAYLISTS'),
          ),
          for (final pair in [
            (SmartView.topRated, 'My Top Rated'),
            (SmartView.recentlyAdded, 'Recently Added'),
            (SmartView.recentlyPlayed, 'Recently Played'),
            (SmartView.neverPlayed, 'Never Played'),
            (SmartView.mostPlayed, 'Most Played'),
          ])
            ListTile(
              dense: true,
              title: Text(pair.$2),
              selected:
                  session.page.value == AppPage.music &&
                  session.library.smart == pair.$1,
              onTap: () {
                session.library.select(newSmart: pair.$1);
                session.page.value = AppPage.music;
              },
            ),
          const Padding(
            padding: EdgeInsets.all(12),
            child: Text('SAVED PLAYLISTS'),
          ),
          for (final playlist
              in session.catalog.value?.playlists ?? <NamedItem>[])
            ListTile(
              dense: true,
              title: Text(
                playlist.name,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
              ),
              onTap: () {
                unawaited(
                  session.execute(Command.loadPlaylist(id: playlist.id)),
                );
                session.page.value = AppPage.queue;
              },
            ),
          ListTile(
            dense: true,
            leading: const Icon(Icons.add, size: 20),
            title: const Text('Save queue'),
            onTap: () => commands.invoke(DesktopAction.savePlaylist),
          ),
        ],
      ),
    ),
  );
}

class StatusArea extends StatelessWidget {
  const StatusArea({super.key, required this.session});
  final OrangeSession session;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: Listenable.merge([
      session.playback,
      session.error,
      session.library,
    ]),
    builder: (context, _) {
      final s = session.playback.value;
      final error = session.error.value ?? s?.error;
      return Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (error != null)
            Material(
              color: Theme.of(context).colorScheme.errorContainer,
              child: Padding(
                padding: const EdgeInsets.symmetric(
                  horizontal: 12,
                  vertical: 6,
                ),
                child: Row(
                  children: [
                    const Icon(Icons.error_outline),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Tooltip(
                        message: error,
                        child: Text(
                          error,
                          maxLines: 3,
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                    ),
                    TextButton(
                      onPressed: () {
                        session.error.value = null;
                        unawaited(
                          session.execute(const Command.dismissError()),
                        );
                      },
                      child: const Text('Dismiss'),
                    ),
                  ],
                ),
              ),
            ),
          const Divider(height: 1),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
            child: Row(
              children: [
                Text('${session.library.value?.total ?? 0} songs'),
                const SizedBox(width: 16),
                Expanded(
                  child: Semantics(
                    liveRegion: true,
                    child: Text(
                      s?.status ?? 'Opening collection…',
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                ),
                if (s?.busy ?? false)
                  const SizedBox(
                    width: 16,
                    height: 16,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  ),
                if (s?.canCancel ?? false)
                  TextButton(
                    onPressed: () =>
                        unawaited(session.execute(const Command.cancel())),
                    child: const Text('Cancel'),
                  ),
              ],
            ),
          ),
        ],
      );
    },
  );
}
