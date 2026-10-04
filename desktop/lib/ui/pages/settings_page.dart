import 'dart:async';

import 'package:flutter/material.dart';

import '../../bridge/api/models.dart';
import '../../state/models.dart';
import '../chrome.dart';
import '../dialogs.dart';

class SettingsPage extends StatelessWidget {
  const SettingsPage({super.key, required this.session});
  final OrangeSession session;
  @override
  Widget build(BuildContext context) => ListenableBuilder(
    listenable: Listenable.merge([session.preferences, session.catalog]),
    builder: (context, _) {
      final settings = session.preferences.value;
      return ListView(
        children: [
          Text('Settings', style: Theme.of(context).textTheme.headlineSmall),
          const SizedBox(height: 16),
          SizedBox(
            width: 250,
            child: DropdownButtonFormField<String>(
              key: ValueKey('theme-${settings?.theme}'),
              initialValue: settings?.theme ?? 'system',
              decoration: const InputDecoration(labelText: 'Appearance'),
              items: const [
                DropdownMenuItem(value: 'system', child: Text('Follow System')),
                DropdownMenuItem(value: 'light', child: Text('Light')),
                DropdownMenuItem(value: 'dark', child: Text('Dark')),
              ],
              onChanged: (v) {
                if (v != null) {
                  unawaited(session.execute(Command.setTheme(theme: v)));
                }
              },
            ),
          ),
          const SizedBox(height: 24),
          Text(
            'Ten-band equalizer',
            style: Theme.of(context).textTheme.titleMedium,
          ),
          const Text(
            'Changes apply to the playing pipeline and are saved automatically.',
          ),
          for (final (index, label) in [
            '29 Hz',
            '59 Hz',
            '119 Hz',
            '237 Hz',
            '474 Hz',
            '947 Hz',
            '1.9 kHz',
            '3.8 kHz',
            '7.6 kHz',
            '15 kHz',
          ].indexed)
            Row(
              children: [
                SizedBox(width: 70, child: Text(label)),
                Expanded(
                  child: CommitSlider(
                    label: 'Equalizer $label',
                    min: -12,
                    max: 12,
                    value: settings?.equalizer[index] ?? 0,
                    commit: (gain) => unawaited(
                      session.execute(
                        Command.setEqualizer(band: index, gain: gain),
                      ),
                    ),
                  ),
                ),
                SizedBox(
                  width: 55,
                  child: Text(
                    '${(settings?.equalizer[index] ?? 0).toStringAsFixed(1)} dB',
                  ),
                ),
              ],
            ),
          const SizedBox(height: 24),
          Text(
            'Collection folders',
            style: Theme.of(context).textTheme.titleMedium,
          ),
          for (final folder
              in session.catalog.value?.directories ?? <NamedItem>[])
            ListTile(
              title: Text(folder.name),
              trailing: IconButton(
                tooltip: 'Remove collection folder',
                icon: const Icon(Icons.remove_circle_outline),
                onPressed: () async {
                  if (await confirmRemoval(
                    context,
                    'Remove folder from collection?',
                    'Indexed tracks from this folder will be removed. Music files stay on disk.',
                  )) {
                    await session.execute(Command.removeFolder(id: folder.id));
                  }
                },
              ),
            ),
        ],
      );
    },
  );
}
