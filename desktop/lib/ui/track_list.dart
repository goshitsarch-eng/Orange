import 'package:flutter/material.dart';

import '../bridge/api/models.dart';
import 'theme.dart';

class TrackList extends StatefulWidget {
  const TrackList({
    super.key,
    required this.tracks,
    required this.play,
    required this.contextMenu,
    this.enqueue,
    this.remove,
    this.cursor,
  });
  final List<TrackDto> tracks;
  final void Function(int) play, contextMenu;
  final void Function(int)? enqueue, remove;
  final int? cursor;
  @override
  State<TrackList> createState() => _TrackListState();
}

class _TrackListState extends State<TrackList> {
  int? _selected;
  @override
  Widget build(BuildContext context) {
    if (widget.tracks.isEmpty) {
      return const Center(child: Text('No tracks to show.'));
    }
    return LayoutBuilder(
      builder: (context, bounds) => Scrollbar(
        child: SingleChildScrollView(
          scrollDirection: Axis.horizontal,
          child: SizedBox(
            width: bounds.maxWidth,
            height: bounds.maxHeight,
            child: Column(
              children: [
                Padding(
                  padding: EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                  child: Row(
                    children: [
                      const SizedBox(width: 32, child: Text('#')),
                      const Expanded(flex: 4, child: Text('Title')),
                      if (bounds.maxWidth >= 680) ...[
                        const Expanded(flex: 3, child: Text('Artist')),
                        const Expanded(flex: 3, child: Text('Album')),
                      ],
                      const SizedBox(width: 56, child: Text('Length')),
                      const SizedBox(width: 108, child: Text('Actions')),
                    ],
                  ),
                ),
                const Divider(height: 1),
                Expanded(
                  child: ListView.builder(
                    itemCount: widget.tracks.length,
                    itemExtent: 44,
                    itemBuilder: (context, index) {
                      final track = widget.tracks[index];
                      Widget action(
                        String label,
                        IconData icon,
                        VoidCallback callback,
                      ) => IconButton(
                        tooltip: label,
                        icon: Icon(icon, size: 19),
                        constraints: const BoxConstraints(
                          minWidth: 32,
                          minHeight: 32,
                        ),
                        padding: const EdgeInsets.all(4),
                        onPressed: callback,
                      );
                      return Material(
                        color: widget.cursor == index || _selected == index
                            ? Theme.of(context).colorScheme.secondaryContainer
                            : Colors.transparent,
                        child: Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 12),
                          child: Row(
                            children: [
                              Expanded(
                                child: InkWell(
                                  onTap: () =>
                                      setState(() => _selected = index),
                                  onDoubleTap: () => widget.play(index),
                                  onSecondaryTap: () =>
                                      widget.contextMenu(index),
                                  child: SizedBox(
                                    height: 44,
                                    child: Row(
                                      children: [
                                        SizedBox(
                                          width: 32,
                                          child: Text(
                                            track.track > 0
                                                ? '${track.track}'
                                                : '',
                                          ),
                                        ),
                                        Expanded(
                                          flex: 4,
                                          child: Tooltip(
                                            message: track.title,
                                            child: Text(
                                              track.title,
                                              maxLines: 1,
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                          ),
                                        ),
                                        if (bounds.maxWidth >= 680) ...[
                                          Expanded(
                                            flex: 3,
                                            child: Text(
                                              track.artist,
                                              maxLines: 1,
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                          ),
                                          Expanded(
                                            flex: 3,
                                            child: Text(
                                              track.album,
                                              maxLines: 1,
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                          ),
                                        ],
                                        SizedBox(
                                          width: 56,
                                          child: Text(
                                            durationLabel(track.duration),
                                          ),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                              ),
                              SizedBox(
                                width: 108,
                                child: Row(
                                  children: [
                                    action(
                                      'Play track',
                                      Icons.play_arrow,
                                      () => widget.play(index),
                                    ),
                                    if (widget.enqueue != null)
                                      action(
                                        'Add to queue',
                                        Icons.add,
                                        () => widget.enqueue!(index),
                                      ),
                                    if (widget.remove != null)
                                      action(
                                        'Remove from queue',
                                        Icons.close,
                                        () => widget.remove!(index),
                                      ),
                                    action(
                                      'Track actions',
                                      Icons.more_horiz,
                                      () => widget.contextMenu(index),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
