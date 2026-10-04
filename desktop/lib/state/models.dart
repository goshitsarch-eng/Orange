import 'dart:async';

import 'package:flutter/foundation.dart';

import '../bridge/api/models.dart';
import '../bridge/api/session.dart';
import '../services/backend.dart';

enum AppPage { music, queue, playlists, radio, files, devices, settings }

class PlaybackModel extends ChangeNotifier {
  PlaybackDto? value;
  void update(PlaybackDto next) {
    value = next;
    notifyListeners();
  }
}

class PreferencesModel extends ChangeNotifier {
  PreferencesDto? value;
  void update(PreferencesDto next) {
    final old = value;
    if (old != null &&
        old.theme == next.theme &&
        old.volume == next.volume &&
        old.repeat == next.repeat &&
        old.shuffle == next.shuffle &&
        old.windowWidth == next.windowWidth &&
        old.windowHeight == next.windowHeight &&
        listEquals(old.equalizer, next.equalizer)) {
      return;
    }
    value = next;
    notifyListeners();
  }
}

class LibraryModel extends ChangeNotifier {
  LibraryModel(this.session, this.onError);
  final Session session;
  final void Function(Object) onError;
  LibraryPage? value;
  String text = '', genre = '', artist = '', album = '';
  SmartView smart = SmartView.all;
  int offset = 0;
  int _request = 0;
  bool _disposed = false;
  Timer? _debounce;
  LibraryQuery get query => LibraryQuery(
    text: text,
    genre: genre,
    artist: artist,
    album: album,
    smart: smart,
    offset: offset,
    limit: 200,
  );
  Future<void> refresh() async {
    if (_disposed) {
      return;
    }
    final request = ++_request;
    try {
      final next = await session.queryLibrary(query: query);
      if (!_disposed && request == _request) {
        value = next;
        notifyListeners();
      }
    } catch (error) {
      if (!_disposed) {
        onError(error);
      }
    }
  }

  void search(String input) {
    text = input;
    offset = 0;
    _debounce?.cancel();
    _debounce = Timer(const Duration(milliseconds: 180), refresh);
  }

  void select({
    String? newGenre,
    String? newArtist,
    String? newAlbum,
    SmartView? newSmart,
  }) {
    if (newGenre != null) {
      genre = newGenre;
      artist = '';
      album = '';
    }
    if (newArtist != null) {
      artist = newArtist;
      album = '';
    }
    if (newAlbum != null) {
      album = newAlbum;
    }
    if (newSmart != null) {
      smart = newSmart;
    }
    offset = 0;
    unawaited(refresh());
  }

  void page(int start) {
    offset = start;
    unawaited(refresh());
  }

  @override
  void dispose() {
    stopRequests();
    super.dispose();
  }

  void stopRequests() {
    _disposed = true;
    _debounce?.cancel();
    ++_request;
  }
}

class QueueModel extends ChangeNotifier {
  List<TrackDto> tracks = [];
  void update(List<TrackDto> next) {
    tracks = next;
    notifyListeners();
  }
}

class CatalogModel extends ChangeNotifier {
  CatalogDto? value;
  void update(CatalogDto next) {
    value = next;
    notifyListeners();
  }
}

/// Coordinates revision updates; presentation models never implement domain rules.
class OrangeSession {
  OrangeSession(this.backend) {
    library = LibraryModel(backend, report);
  }
  final Session backend;
  final playback = PlaybackModel();
  final preferences = PreferencesModel();
  late final LibraryModel library;
  final queue = QueueModel();
  final catalog = CatalogModel();
  final page = ValueNotifier(AppPage.music);
  final error = ValueNotifier<String?>(null);
  Timer? _timer;
  bool _polling = false, _closed = false;
  Future<void>? _closeFuture;
  int _libraryRevision = -1, _queueRevision = -1, _catalogRevision = -1;
  void report(Object failure) {
    if (!_closed) {
      debugPrint('Orange Flutter/bridge failure: ${failure.runtimeType}');
      error.value = failureMessage(failure);
    }
  }

  Future<void> start() async {
    await poll();
    _timer = Timer.periodic(
      const Duration(milliseconds: 250),
      (_) => unawaited(poll()),
    );
  }

  Future<void> poll() async {
    if (_polling || _closed) {
      return;
    }
    _polling = true;
    try {
      final next = await backend.playback();
      if (_closed) {
        return;
      }
      preferences.update(next.preferences);
      playback.update(next);
      if (_libraryRevision != next.libraryRevision) {
        _libraryRevision = next.libraryRevision;
        await library.refresh();
      }
      if (_queueRevision != next.queueRevision) {
        _queueRevision = next.queueRevision;
        final rows = await backend.queue();
        if (!_closed) {
          queue.update(rows);
        }
      }
      if (_catalogRevision != next.catalogRevision) {
        _catalogRevision = next.catalogRevision;
        final rows = await backend.catalog();
        if (!_closed) {
          catalog.update(rows);
        }
      }
    } catch (failure) {
      report(failure);
    } finally {
      _polling = false;
    }
  }

  Future<void> execute(Command command) async {
    if (_closed) {
      return;
    }
    try {
      await backend.dispatch(command: command);
    } catch (failure) {
      report(failure);
    }
  }

  Future<void> close() => _closeFuture ??= _finishClose();

  Future<void> _finishClose() async {
    _closed = true;
    _timer?.cancel();
    library.stopRequests();
    try {
      await backend.close();
    } catch (_) {
      _closeFuture = null;
      rethrow;
    }
    library.dispose();
    playback.dispose();
    preferences.dispose();
    queue.dispose();
    catalog.dispose();
    page.dispose();
    error.dispose();
    backend.dispose();
  }
}
