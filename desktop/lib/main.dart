import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:desktop_drop/desktop_drop.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import 'bridge/api/models.dart';
import 'commands/desktop_commands.dart';
import 'platform/desktop_platform.dart';
import 'services/backend.dart';
import 'state/models.dart';
import 'ui/chrome.dart';
import 'ui/dialogs.dart';
import 'ui/pages/catalog_pages.dart';
import 'ui/pages/library_page.dart';
import 'ui/pages/queue_page.dart';
import 'ui/pages/settings_page.dart';
import 'ui/theme.dart';

Future<void> main(List<String> arguments) async {
  final cliArguments = arguments.where((arg) => arg != '--ui').toList();
  if (cliArguments.any((arg) => arg.startsWith('-'))) {
    final executable = Platform.isMacOS
        ? '${File(Platform.resolvedExecutable).parent.path}/orange-cli'
        : '${File(Platform.resolvedExecutable).parent.path}/orange-cli${Platform.isWindows ? '.exe' : ''}';
    try {
      final child = await Process.start(
        executable,
        cliArguments,
        mode: ProcessStartMode.inheritStdio,
      );
      exit(await child.exitCode);
    } catch (error) {
      stderr.writeln(
        'Orange could not start its command-line helper: ${error.runtimeType}',
      );
      exit(1);
    }
  }
  WidgetsFlutterBinding.ensureInitialized();
  try {
    final platform = DesktopPlatform();
    await platform.initialize();
    final session = OrangeSession(await connectBackend(cliArguments));
    await session.start();
    final settings = session.preferences.value;
    if (settings != null) {
      await windowManager.setSize(
        Size(settings.windowWidth.toDouble(), settings.windowHeight.toDouble()),
      );
    }
    runApp(OrangeDesktop(session: session, platform: platform));
    await windowManager.show();
    await windowManager.focus();
  } catch (error, stack) {
    debugPrint('Orange startup failed: ${error.runtimeType}\n$stack');
    runApp(
      MaterialApp(
        theme: orangeTheme(Brightness.light),
        home: Scaffold(
          body: Center(
            child: Padding(
              padding: const EdgeInsets.all(32),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const Icon(Icons.error_outline, size: 48),
                  const SizedBox(height: 16),
                  const Text('Orange could not start'),
                  const SizedBox(height: 12),
                  SelectableText(failureMessage(error)),
                  const SizedBox(height: 16),
                  FilledButton(
                    onPressed: () => exit(1),
                    child: const Text('Close'),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
    await windowManager.show();
  }
}

class OrangeDesktop extends StatefulWidget {
  const OrangeDesktop({
    super.key,
    required this.session,
    required this.platform,
  });
  final OrangeSession session;
  final DesktopPlatform platform;
  @override
  State<OrangeDesktop> createState() => _OrangeDesktopState();
}

class _OrangeDesktopState extends State<OrangeDesktop> with WindowListener {
  Timer? _resize;
  bool _quitting = false;
  String? _closeError;
  @override
  void initState() {
    super.initState();
    windowManager.addListener(this);
    widget.session.playback.addListener(_backendLifecycle);
  }

  void _backendLifecycle() {
    if (widget.session.playback.value?.shutdown == true) {
      unawaited(_quit());
    }
  }

  @override
  void onWindowClose() {
    unawaited(_quit());
  }

  @override
  void onWindowResized() {
    _resize?.cancel();
    _resize = Timer(const Duration(milliseconds: 350), () async {
      try {
        final size = await windowManager.getSize();
        await widget.session.execute(
          Command.saveWindowSize(
            width: size.width.round(),
            height: size.height.round(),
          ),
        );
      } catch (error) {
        widget.session.report(error);
      }
    });
  }

  Future<void> _quit() async {
    if (_quitting) {
      return;
    }
    _quitting = true;
    _resize?.cancel();
    widget.session.playback.removeListener(_backendLifecycle);
    try {
      await widget.session.close();
      await windowManager.destroy();
    } catch (error) {
      _quitting = false;
      if (mounted) {
        setState(() => _closeError = failureMessage(error));
      }
    }
  }

  @override
  void dispose() {
    _resize?.cancel();
    windowManager.removeListener(this);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: widget.session.preferences,
    builder: (context, _) => MaterialApp(
      title: 'Orange Music Player',
      debugShowCheckedModeBanner: false,
      theme: orangeTheme(Brightness.light),
      darkTheme: orangeTheme(Brightness.dark),
      themeMode: switch (widget.session.preferences.value?.theme) {
        'dark' => ThemeMode.dark,
        'light' => ThemeMode.light,
        _ => ThemeMode.system,
      },
      home: _closeError != null
          ? Scaffold(
              body: Center(
                child: Padding(
                  padding: const EdgeInsets.all(32),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      const Text('Orange could not finish saving and closing.'),
                      const SizedBox(height: 16),
                      SelectableText(_closeError!),
                      const SizedBox(height: 16),
                      FilledButton(
                        onPressed: _quit,
                        child: const Text('Retry Close'),
                      ),
                    ],
                  ),
                ),
              ),
            )
          : DesktopShell(
              session: widget.session,
              platform: widget.platform,
              quit: _quit,
            ),
    ),
  );
}

class DesktopShell extends StatelessWidget {
  const DesktopShell({
    super.key,
    required this.session,
    required this.platform,
    required this.quit,
  });
  final OrangeSession session;
  final DesktopPlatform platform;
  final Future<void> Function() quit;
  @override
  Widget build(BuildContext context) {
    final commands = DesktopCommands(
      session,
      platform,
      (action) =>
          unawaited(showApplicationDialog(context, action, session, platform)),
      quit,
    );
    void track(TrackDto data, bool library) =>
        unawaited(showTrackActions(context, data, library, session, platform));
    final primary = platform.usesCommandKey;
    SingleActivator key(LogicalKeyboardKey code, {bool shift = false}) =>
        SingleActivator(code, control: !primary, meta: primary, shift: shift);
    return Shortcuts(
      shortcuts: {
        key(LogicalKeyboardKey.keyO): const DesktopIntent(DesktopAction.open),
        key(LogicalKeyboardKey.comma): const DesktopIntent(
          DesktopAction.preferences,
        ),
        key(LogicalKeyboardKey.keyQ): const DesktopIntent(DesktopAction.quit),
        key(LogicalKeyboardKey.keyZ, shift: true): const DesktopIntent(
          DesktopAction.redo,
        ),
        key(LogicalKeyboardKey.keyZ): const DesktopIntent(DesktopAction.undo),
        key(LogicalKeyboardKey.keyP): const DesktopIntent(
          DesktopAction.playPause,
        ),
      },
      child: Actions(
        actions: {
          DesktopIntent: CallbackAction<DesktopIntent>(
            onInvoke: (intent) {
              commands.invoke(intent.command);
              return null;
            },
          ),
        },
        child: FocusTraversalGroup(
          child: Focus(
            autofocus: true,
            child: Scaffold(
              body: DropTarget(
                onDragDone: (details) {
                  final paths = details.files.map((file) => file.path).toList();
                  if (paths.isNotEmpty) {
                    unawaited(session.execute(Command.openFiles(paths: paths)));
                  }
                },
                child: Column(
                  children: [
                    AppMenus(commands: commands),
                    Transport(session: session, commands: commands),
                    const Divider(height: 1),
                    Expanded(
                      child: Row(
                        children: [
                          SizedBox(
                            width: 190,
                            child: Sidebar(
                              session: session,
                              commands: commands,
                            ),
                          ),
                          const VerticalDivider(width: 1),
                          Expanded(
                            child: Padding(
                              padding: const EdgeInsets.all(16),
                              child: LayoutBuilder(
                                builder: (context, bounds) =>
                                    ValueListenableBuilder(
                                      valueListenable: session.page,
                                      builder: (context, page, _) {
                                        final compact =
                                            bounds.maxHeight < 380 ||
                                            bounds.maxWidth < 620;
                                        if (page == AppPage.music) {
                                          return LibraryPageView(
                                            session: session,
                                            commands: commands,
                                            trackActions: track,
                                            compact: compact,
                                          );
                                        }
                                        final content = switch (page) {
                                          AppPage.music =>
                                            const SizedBox.shrink(),
                                          AppPage.queue => QueuePage(
                                            session: session,
                                            commands: commands,
                                            trackActions: track,
                                          ),
                                          AppPage.playlists => PlaylistsPage(
                                            session: session,
                                            commands: commands,
                                          ),
                                          AppPage.radio => RadioPage(
                                            session: session,
                                          ),
                                          AppPage.files => FilesPage(
                                            session: session,
                                            platform: platform,
                                          ),
                                          AppPage.devices => DevicesPage(
                                            session: session,
                                            platform: platform,
                                          ),
                                          AppPage.settings => SettingsPage(
                                            session: session,
                                          ),
                                        };
                                        return Scrollbar(
                                          child: SingleChildScrollView(
                                            child: SizedBox(
                                              height: math.max(
                                                bounds.maxHeight,
                                                420,
                                              ),
                                              child: content,
                                            ),
                                          ),
                                        );
                                      },
                                    ),
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                    StatusArea(session: session),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
