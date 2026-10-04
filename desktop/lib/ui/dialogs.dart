import 'dart:async';

import 'package:flutter/material.dart';
import 'package:path/path.dart' as p;

import '../bridge/api/models.dart';
import '../bridge/api/session.dart';
import '../commands/desktop_commands.dart';
import '../platform/desktop_platform.dart';
import '../state/models.dart';
import 'chrome.dart';

Future<void> showApplicationDialog(
  BuildContext context,
  DesktopAction action,
  OrangeSession session,
  DesktopPlatform platform,
) async {
  switch (action) {
    case DesktopAction.savePlaylist:
      final name = await showDialog<String>(
        context: context,
        builder: (context) => const NameDialog(),
      );
      if (name != null) {
        await session.execute(Command.savePlaylist(name: name));
      }
    case DesktopAction.about:
      if (!context.mounted) {
        return;
      }
      await showDialog<void>(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text('Orange Music Player'),
          content: const SizedBox(
            width: 420,
            child: Text(
              'Orange 3.1.0-alpha.1\nMade by Gosh\n\nYour music, your collection.\nGPL-3.0-or-later.',
            ),
          ),
          actions: [
            TextButton(
              onPressed: () async {
                try {
                  await platform.openProject();
                } catch (error) {
                  session.report(error);
                }
              },
              child: const Text('Project page'),
            ),
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Close'),
            ),
          ],
        ),
      );
    case DesktopAction.lyrics:
      if (!context.mounted) {
        return;
      }
      await showDialog<void>(
        context: context,
        builder: (context) => ListenableBuilder(
          listenable: session.catalog,
          builder: (context, _) => AlertDialog(
            title: const Text('Lyrics'),
            content: SizedBox(
              width: 560,
              height: 320,
              child: SingleChildScrollView(
                child: SelectableText(
                  session.catalog.value?.lyrics ??
                      'No lyrics loaded for the current track.',
                ),
              ),
            ),
            actions: [
              TextButton(
                onPressed: () =>
                    unawaited(session.execute(const Command.fetchLyrics())),
                child: const Text('Look up lyrics'),
              ),
              TextButton(
                onPressed: () => Navigator.pop(context),
                child: const Text('Close'),
              ),
            ],
          ),
        ),
      );
    default:
      return;
  }
}

class NameDialog extends StatefulWidget {
  const NameDialog({super.key});
  @override
  State<NameDialog> createState() => _NameDialogState();
}

class _NameDialogState extends State<NameDialog> {
  final _name = TextEditingController();
  final _form = GlobalKey<FormState>();
  @override
  void dispose() {
    _name.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => AlertDialog(
    title: const Text('Save queue as playlist'),
    content: Form(
      key: _form,
      child: TextFormField(
        controller: _name,
        autofocus: true,
        maxLength: 200,
        decoration: const InputDecoration(labelText: 'Playlist name'),
        validator: (value) => value == null || value.trim().isEmpty
            ? 'Enter a playlist name.'
            : null,
        onFieldSubmitted: (_) => _save(),
      ),
    ),
    actions: [
      TextButton(
        onPressed: () => Navigator.pop(context),
        child: const Text('Cancel'),
      ),
      FilledButton(onPressed: _save, child: const Text('Save')),
    ],
  );
  void _save() {
    if (_form.currentState!.validate()) {
      Navigator.pop(context, _name.text.trim());
    }
  }
}

Future<bool> confirmRemoval(
  BuildContext context,
  String title,
  String message,
) async =>
    await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(title),
        content: Text(message),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Remove'),
          ),
        ],
      ),
    ) ??
    false;

Future<void> showTrackActions(
  BuildContext context,
  TrackDto track,
  bool inLibrary,
  OrangeSession session,
  DesktopPlatform platform,
) async {
  final file = Uri.tryParse(track.url);
  final local = file?.scheme == 'file';
  await showDialog<void>(
    context: context,
    builder: (context) => AlertDialog(
      title: Text(track.title),
      content: SizedBox(
        width: 420,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text('${track.artist} · ${track.album}'),
            const SizedBox(height: 12),
            if (inLibrary)
              CommitSlider(
                label: 'Track rating',
                value: track.rating.clamp(0, 1),
                max: 1,
                commit: (rating) => unawaited(
                  session.execute(Command.rate(url: track.url, rating: rating)),
                ),
              ),
            if (local)
              ListTile(
                leading: const Icon(Icons.edit),
                title: const Text('Edit Tags…'),
                onTap: () {
                  Navigator.pop(context);
                  unawaited(
                    showDialog<void>(
                      context: context,
                      builder: (context) =>
                          TagDialog(track: track, session: session),
                    ),
                  );
                },
              ),
            if (local)
              ListTile(
                leading: const Icon(Icons.transform),
                title: const Text('Convert Audio…'),
                onTap: () {
                  Navigator.pop(context);
                  unawaited(
                    showDialog<void>(
                      context: context,
                      builder: (context) => ConversionDialog(
                        track: track,
                        session: session,
                        platform: platform,
                      ),
                    ),
                  );
                },
              ),
            if (local)
              ListTile(
                leading: const Icon(Icons.folder_open),
                title: const Text('Open containing folder'),
                onTap: () async {
                  try {
                    await platform.openFolder(
                      Uri.directory(p.dirname(file!.toFilePath())),
                    );
                  } catch (error) {
                    session.report(error);
                  }
                },
              ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Close'),
        ),
      ],
    ),
  );
}

class TagDialog extends StatefulWidget {
  const TagDialog({super.key, required this.track, required this.session});
  final TrackDto track;
  final OrangeSession session;
  @override
  State<TagDialog> createState() => _TagDialogState();
}

class _TagDialogState extends State<TagDialog> {
  late final _fields = <String, TextEditingController>{
    'Title': TextEditingController(text: widget.track.title),
    'Artist': TextEditingController(text: widget.track.artist),
    'Album': TextEditingController(text: widget.track.album),
    'Genre': TextEditingController(text: widget.track.genre),
    'Year': TextEditingController(
      text: widget.track.year > 0 ? '${widget.track.year}' : '',
    ),
    'Track': TextEditingController(
      text: widget.track.track > 0 ? '${widget.track.track}' : '',
    ),
  };
  final _form = GlobalKey<FormState>();
  @override
  void dispose() {
    for (final field in _fields.values) {
      field.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => AlertDialog(
    title: const Text('Edit Tags'),
    content: SizedBox(
      width: 460,
      child: SingleChildScrollView(
        child: Form(
          key: _form,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              for (final entry in _fields.entries)
                Padding(
                  padding: const EdgeInsets.only(bottom: 12),
                  child: TextFormField(
                    controller: entry.value,
                    decoration: InputDecoration(labelText: entry.key),
                    validator: (text) {
                      if (['Year', 'Track'].contains(entry.key) &&
                          text != null &&
                          text.isNotEmpty &&
                          (int.tryParse(text) == null ||
                              int.parse(text) <= 0)) {
                        return 'Enter a positive whole number.';
                      }
                      return null;
                    },
                  ),
                ),
            ],
          ),
        ),
      ),
    ),
    actions: [
      TextButton(
        onPressed: () => Navigator.pop(context),
        child: const Text('Cancel'),
      ),
      FilledButton(
        onPressed: () {
          if (!_form.currentState!.validate()) {
            return;
          }
          unawaited(
            widget.session.execute(
              Command.saveTags(
                edit: TagEdit(
                  path: Uri.parse(widget.track.url).toFilePath(),
                  title: _fields['Title']!.text,
                  artist: _fields['Artist']!.text,
                  album: _fields['Album']!.text,
                  genre: _fields['Genre']!.text,
                  year: int.tryParse(_fields['Year']!.text),
                  track: int.tryParse(_fields['Track']!.text),
                ),
              ),
            ),
          );
          Navigator.pop(context);
        },
        child: const Text('Save Tags'),
      ),
    ],
  );
}

class ConversionDialog extends StatefulWidget {
  const ConversionDialog({
    super.key,
    required this.track,
    required this.session,
    required this.platform,
  });
  final TrackDto track;
  final OrangeSession session;
  final DesktopPlatform platform;
  @override
  State<ConversionDialog> createState() => _ConversionDialogState();
}

class _ConversionDialogState extends State<ConversionDialog> {
  List<ConversionTargetDto>? _targets;
  String? _format;
  String? _error;
  bool _choosing = false;
  @override
  void initState() {
    super.initState();
    unawaited(_load());
  }

  Future<void> _load() async {
    try {
      final targets = await conversionTargets();
      if (!mounted) {
        return;
      }
      final available = targets.where((target) => target.available);
      setState(() {
        _targets = targets;
        _format = available.any((target) => target.name == 'FLAC')
            ? 'FLAC'
            : available.firstOrNull?.name;
      });
    } catch (error) {
      if (mounted) {
        setState(() => _error = error.toString());
      }
    }
  }

  @override
  Widget build(BuildContext context) => AlertDialog(
    title: const Text('Convert Audio'),
    content: SizedBox(
      width: 420,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(widget.track.title),
          const SizedBox(height: 16),
          if (_error != null) Text(_error!),
          if (_targets == null && _error == null)
            const LinearProgressIndicator(),
          if (_targets != null)
            DropdownButtonFormField<String>(
              initialValue: _format,
              decoration: const InputDecoration(labelText: 'Output format'),
              items: [
                for (final target in _targets!)
                  DropdownMenuItem(
                    value: target.name,
                    enabled: target.available,
                    child: Text(
                      target.available
                          ? target.name
                          : '${target.name} (encoder unavailable)',
                    ),
                  ),
              ],
              onChanged: (value) => setState(() => _format = value),
            ),
          const SizedBox(height: 12),
          const Text(
            'The original is preserved. Choose a new destination; existing output files are not overwritten.',
          ),
        ],
      ),
    ),
    actions: [
      TextButton(
        onPressed: () => Navigator.pop(context),
        child: const Text('Cancel'),
      ),
      FilledButton(
        onPressed: _format == null || _choosing
            ? null
            : () async {
                setState(() => _choosing = true);
                try {
                  final source = Uri.parse(widget.track.url).toFilePath();
                  final target = _targets!.firstWhere(
                    (target) => target.name == _format,
                  );
                  final destination = await widget.platform.save(
                    '${p.basenameWithoutExtension(source)}.${target.extension_}',
                  );
                  if (destination != null) {
                    await widget.session.execute(
                      Command.convertAudio(
                        source: source,
                        format: target.name,
                        destination: destination,
                      ),
                    );
                    if (context.mounted) {
                      Navigator.pop(context);
                    }
                  }
                } catch (error) {
                  widget.session.report(error);
                } finally {
                  if (mounted) {
                    setState(() => _choosing = false);
                  }
                }
              },
        child: const Text('Choose Output File…'),
      ),
    ],
  );
}
