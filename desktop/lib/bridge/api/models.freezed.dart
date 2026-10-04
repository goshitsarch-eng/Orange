// GENERATED CODE - DO NOT MODIFY BY HAND
// coverage:ignore-file
// ignore_for_file: type=lint, type=warning, deprecated_member_use, deprecated_member_use_from_same_package
// ignore_for_file: unused_element, deprecated_member_use, deprecated_member_use_from_same_package, use_function_type_syntax_for_parameters, unnecessary_const, avoid_init_to_null, invalid_override_different_default_values_named, prefer_expression_function_bodies, annotate_overrides, invalid_annotation_target, unnecessary_question_mark

part of 'models.dart';

// **************************************************************************
// FreezedGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// dart format off
T _$identity<T>(T value) => value;
/// @nodoc
mixin _$Command {





@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command()';
}


}

/// @nodoc
class $CommandCopyWith<$Res>  {
$CommandCopyWith(Command _, $Res Function(Command) __);
}


/// Adds pattern-matching-related methods to [Command].
extension CommandPatterns on Command {
/// A variant of `map` that fallback to returning `orElse`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeMap<TResult extends Object?>({TResult Function( Command_PlayPause value)?  playPause,TResult Function( Command_Stop value)?  stop,TResult Function( Command_Next value)?  next,TResult Function( Command_Previous value)?  previous,TResult Function( Command_StopAfterCurrent value)?  stopAfterCurrent,TResult Function( Command_Seek value)?  seek,TResult Function( Command_SetVolume value)?  setVolume,TResult Function( Command_PlayLibrary value)?  playLibrary,TResult Function( Command_Enqueue value)?  enqueue,TResult Function( Command_PlayQueue value)?  playQueue,TResult Function( Command_RemoveQueue value)?  removeQueue,TResult Function( Command_ClearQueue value)?  clearQueue,TResult Function( Command_SetRepeat value)?  setRepeat,TResult Function( Command_SetShuffle value)?  setShuffle,TResult Function( Command_SetEqualizer value)?  setEqualizer,TResult Function( Command_SetTheme value)?  setTheme,TResult Function( Command_SaveWindowSize value)?  saveWindowSize,TResult Function( Command_AddFolder value)?  addFolder,TResult Function( Command_RemoveFolder value)?  removeFolder,TResult Function( Command_Rescan value)?  rescan,TResult Function( Command_LoadPlaylist value)?  loadPlaylist,TResult Function( Command_SavePlaylist value)?  savePlaylist,TResult Function( Command_DeletePlaylist value)?  deletePlaylist,TResult Function( Command_OpenFiles value)?  openFiles,TResult Function( Command_OpenUris value)?  openUris,TResult Function( Command_PlayFolder value)?  playFolder,TResult Function( Command_BrowseFolder value)?  browseFolder,TResult Function( Command_ExportPlaylist value)?  exportPlaylist,TResult Function( Command_ImportPlaylist value)?  importPlaylist,TResult Function( Command_AddStation value)?  addStation,TResult Function( Command_RemoveStation value)?  removeStation,TResult Function( Command_UndoQueue value)?  undoQueue,TResult Function( Command_RedoQueue value)?  redoQueue,TResult Function( Command_SaveTags value)?  saveTags,TResult Function( Command_ConvertAudio value)?  convertAudio,TResult Function( Command_CopyQueue value)?  copyQueue,TResult Function( Command_Cancel value)?  cancel,TResult Function( Command_SearchRadio value)?  searchRadio,TResult Function( Command_FetchLyrics value)?  fetchLyrics,TResult Function( Command_Rate value)?  rate,TResult Function( Command_DismissError value)?  dismissError,required TResult orElse(),}){
final _that = this;
switch (_that) {
case Command_PlayPause() when playPause != null:
return playPause(_that);case Command_Stop() when stop != null:
return stop(_that);case Command_Next() when next != null:
return next(_that);case Command_Previous() when previous != null:
return previous(_that);case Command_StopAfterCurrent() when stopAfterCurrent != null:
return stopAfterCurrent(_that);case Command_Seek() when seek != null:
return seek(_that);case Command_SetVolume() when setVolume != null:
return setVolume(_that);case Command_PlayLibrary() when playLibrary != null:
return playLibrary(_that);case Command_Enqueue() when enqueue != null:
return enqueue(_that);case Command_PlayQueue() when playQueue != null:
return playQueue(_that);case Command_RemoveQueue() when removeQueue != null:
return removeQueue(_that);case Command_ClearQueue() when clearQueue != null:
return clearQueue(_that);case Command_SetRepeat() when setRepeat != null:
return setRepeat(_that);case Command_SetShuffle() when setShuffle != null:
return setShuffle(_that);case Command_SetEqualizer() when setEqualizer != null:
return setEqualizer(_that);case Command_SetTheme() when setTheme != null:
return setTheme(_that);case Command_SaveWindowSize() when saveWindowSize != null:
return saveWindowSize(_that);case Command_AddFolder() when addFolder != null:
return addFolder(_that);case Command_RemoveFolder() when removeFolder != null:
return removeFolder(_that);case Command_Rescan() when rescan != null:
return rescan(_that);case Command_LoadPlaylist() when loadPlaylist != null:
return loadPlaylist(_that);case Command_SavePlaylist() when savePlaylist != null:
return savePlaylist(_that);case Command_DeletePlaylist() when deletePlaylist != null:
return deletePlaylist(_that);case Command_OpenFiles() when openFiles != null:
return openFiles(_that);case Command_OpenUris() when openUris != null:
return openUris(_that);case Command_PlayFolder() when playFolder != null:
return playFolder(_that);case Command_BrowseFolder() when browseFolder != null:
return browseFolder(_that);case Command_ExportPlaylist() when exportPlaylist != null:
return exportPlaylist(_that);case Command_ImportPlaylist() when importPlaylist != null:
return importPlaylist(_that);case Command_AddStation() when addStation != null:
return addStation(_that);case Command_RemoveStation() when removeStation != null:
return removeStation(_that);case Command_UndoQueue() when undoQueue != null:
return undoQueue(_that);case Command_RedoQueue() when redoQueue != null:
return redoQueue(_that);case Command_SaveTags() when saveTags != null:
return saveTags(_that);case Command_ConvertAudio() when convertAudio != null:
return convertAudio(_that);case Command_CopyQueue() when copyQueue != null:
return copyQueue(_that);case Command_Cancel() when cancel != null:
return cancel(_that);case Command_SearchRadio() when searchRadio != null:
return searchRadio(_that);case Command_FetchLyrics() when fetchLyrics != null:
return fetchLyrics(_that);case Command_Rate() when rate != null:
return rate(_that);case Command_DismissError() when dismissError != null:
return dismissError(_that);case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// Callbacks receives the raw object, upcasted.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case final Subclass2 value:
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult map<TResult extends Object?>({required TResult Function( Command_PlayPause value)  playPause,required TResult Function( Command_Stop value)  stop,required TResult Function( Command_Next value)  next,required TResult Function( Command_Previous value)  previous,required TResult Function( Command_StopAfterCurrent value)  stopAfterCurrent,required TResult Function( Command_Seek value)  seek,required TResult Function( Command_SetVolume value)  setVolume,required TResult Function( Command_PlayLibrary value)  playLibrary,required TResult Function( Command_Enqueue value)  enqueue,required TResult Function( Command_PlayQueue value)  playQueue,required TResult Function( Command_RemoveQueue value)  removeQueue,required TResult Function( Command_ClearQueue value)  clearQueue,required TResult Function( Command_SetRepeat value)  setRepeat,required TResult Function( Command_SetShuffle value)  setShuffle,required TResult Function( Command_SetEqualizer value)  setEqualizer,required TResult Function( Command_SetTheme value)  setTheme,required TResult Function( Command_SaveWindowSize value)  saveWindowSize,required TResult Function( Command_AddFolder value)  addFolder,required TResult Function( Command_RemoveFolder value)  removeFolder,required TResult Function( Command_Rescan value)  rescan,required TResult Function( Command_LoadPlaylist value)  loadPlaylist,required TResult Function( Command_SavePlaylist value)  savePlaylist,required TResult Function( Command_DeletePlaylist value)  deletePlaylist,required TResult Function( Command_OpenFiles value)  openFiles,required TResult Function( Command_OpenUris value)  openUris,required TResult Function( Command_PlayFolder value)  playFolder,required TResult Function( Command_BrowseFolder value)  browseFolder,required TResult Function( Command_ExportPlaylist value)  exportPlaylist,required TResult Function( Command_ImportPlaylist value)  importPlaylist,required TResult Function( Command_AddStation value)  addStation,required TResult Function( Command_RemoveStation value)  removeStation,required TResult Function( Command_UndoQueue value)  undoQueue,required TResult Function( Command_RedoQueue value)  redoQueue,required TResult Function( Command_SaveTags value)  saveTags,required TResult Function( Command_ConvertAudio value)  convertAudio,required TResult Function( Command_CopyQueue value)  copyQueue,required TResult Function( Command_Cancel value)  cancel,required TResult Function( Command_SearchRadio value)  searchRadio,required TResult Function( Command_FetchLyrics value)  fetchLyrics,required TResult Function( Command_Rate value)  rate,required TResult Function( Command_DismissError value)  dismissError,}){
final _that = this;
switch (_that) {
case Command_PlayPause():
return playPause(_that);case Command_Stop():
return stop(_that);case Command_Next():
return next(_that);case Command_Previous():
return previous(_that);case Command_StopAfterCurrent():
return stopAfterCurrent(_that);case Command_Seek():
return seek(_that);case Command_SetVolume():
return setVolume(_that);case Command_PlayLibrary():
return playLibrary(_that);case Command_Enqueue():
return enqueue(_that);case Command_PlayQueue():
return playQueue(_that);case Command_RemoveQueue():
return removeQueue(_that);case Command_ClearQueue():
return clearQueue(_that);case Command_SetRepeat():
return setRepeat(_that);case Command_SetShuffle():
return setShuffle(_that);case Command_SetEqualizer():
return setEqualizer(_that);case Command_SetTheme():
return setTheme(_that);case Command_SaveWindowSize():
return saveWindowSize(_that);case Command_AddFolder():
return addFolder(_that);case Command_RemoveFolder():
return removeFolder(_that);case Command_Rescan():
return rescan(_that);case Command_LoadPlaylist():
return loadPlaylist(_that);case Command_SavePlaylist():
return savePlaylist(_that);case Command_DeletePlaylist():
return deletePlaylist(_that);case Command_OpenFiles():
return openFiles(_that);case Command_OpenUris():
return openUris(_that);case Command_PlayFolder():
return playFolder(_that);case Command_BrowseFolder():
return browseFolder(_that);case Command_ExportPlaylist():
return exportPlaylist(_that);case Command_ImportPlaylist():
return importPlaylist(_that);case Command_AddStation():
return addStation(_that);case Command_RemoveStation():
return removeStation(_that);case Command_UndoQueue():
return undoQueue(_that);case Command_RedoQueue():
return redoQueue(_that);case Command_SaveTags():
return saveTags(_that);case Command_ConvertAudio():
return convertAudio(_that);case Command_CopyQueue():
return copyQueue(_that);case Command_Cancel():
return cancel(_that);case Command_SearchRadio():
return searchRadio(_that);case Command_FetchLyrics():
return fetchLyrics(_that);case Command_Rate():
return rate(_that);case Command_DismissError():
return dismissError(_that);}
}
/// A variant of `map` that fallback to returning `null`.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case final Subclass value:
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? mapOrNull<TResult extends Object?>({TResult? Function( Command_PlayPause value)?  playPause,TResult? Function( Command_Stop value)?  stop,TResult? Function( Command_Next value)?  next,TResult? Function( Command_Previous value)?  previous,TResult? Function( Command_StopAfterCurrent value)?  stopAfterCurrent,TResult? Function( Command_Seek value)?  seek,TResult? Function( Command_SetVolume value)?  setVolume,TResult? Function( Command_PlayLibrary value)?  playLibrary,TResult? Function( Command_Enqueue value)?  enqueue,TResult? Function( Command_PlayQueue value)?  playQueue,TResult? Function( Command_RemoveQueue value)?  removeQueue,TResult? Function( Command_ClearQueue value)?  clearQueue,TResult? Function( Command_SetRepeat value)?  setRepeat,TResult? Function( Command_SetShuffle value)?  setShuffle,TResult? Function( Command_SetEqualizer value)?  setEqualizer,TResult? Function( Command_SetTheme value)?  setTheme,TResult? Function( Command_SaveWindowSize value)?  saveWindowSize,TResult? Function( Command_AddFolder value)?  addFolder,TResult? Function( Command_RemoveFolder value)?  removeFolder,TResult? Function( Command_Rescan value)?  rescan,TResult? Function( Command_LoadPlaylist value)?  loadPlaylist,TResult? Function( Command_SavePlaylist value)?  savePlaylist,TResult? Function( Command_DeletePlaylist value)?  deletePlaylist,TResult? Function( Command_OpenFiles value)?  openFiles,TResult? Function( Command_OpenUris value)?  openUris,TResult? Function( Command_PlayFolder value)?  playFolder,TResult? Function( Command_BrowseFolder value)?  browseFolder,TResult? Function( Command_ExportPlaylist value)?  exportPlaylist,TResult? Function( Command_ImportPlaylist value)?  importPlaylist,TResult? Function( Command_AddStation value)?  addStation,TResult? Function( Command_RemoveStation value)?  removeStation,TResult? Function( Command_UndoQueue value)?  undoQueue,TResult? Function( Command_RedoQueue value)?  redoQueue,TResult? Function( Command_SaveTags value)?  saveTags,TResult? Function( Command_ConvertAudio value)?  convertAudio,TResult? Function( Command_CopyQueue value)?  copyQueue,TResult? Function( Command_Cancel value)?  cancel,TResult? Function( Command_SearchRadio value)?  searchRadio,TResult? Function( Command_FetchLyrics value)?  fetchLyrics,TResult? Function( Command_Rate value)?  rate,TResult? Function( Command_DismissError value)?  dismissError,}){
final _that = this;
switch (_that) {
case Command_PlayPause() when playPause != null:
return playPause(_that);case Command_Stop() when stop != null:
return stop(_that);case Command_Next() when next != null:
return next(_that);case Command_Previous() when previous != null:
return previous(_that);case Command_StopAfterCurrent() when stopAfterCurrent != null:
return stopAfterCurrent(_that);case Command_Seek() when seek != null:
return seek(_that);case Command_SetVolume() when setVolume != null:
return setVolume(_that);case Command_PlayLibrary() when playLibrary != null:
return playLibrary(_that);case Command_Enqueue() when enqueue != null:
return enqueue(_that);case Command_PlayQueue() when playQueue != null:
return playQueue(_that);case Command_RemoveQueue() when removeQueue != null:
return removeQueue(_that);case Command_ClearQueue() when clearQueue != null:
return clearQueue(_that);case Command_SetRepeat() when setRepeat != null:
return setRepeat(_that);case Command_SetShuffle() when setShuffle != null:
return setShuffle(_that);case Command_SetEqualizer() when setEqualizer != null:
return setEqualizer(_that);case Command_SetTheme() when setTheme != null:
return setTheme(_that);case Command_SaveWindowSize() when saveWindowSize != null:
return saveWindowSize(_that);case Command_AddFolder() when addFolder != null:
return addFolder(_that);case Command_RemoveFolder() when removeFolder != null:
return removeFolder(_that);case Command_Rescan() when rescan != null:
return rescan(_that);case Command_LoadPlaylist() when loadPlaylist != null:
return loadPlaylist(_that);case Command_SavePlaylist() when savePlaylist != null:
return savePlaylist(_that);case Command_DeletePlaylist() when deletePlaylist != null:
return deletePlaylist(_that);case Command_OpenFiles() when openFiles != null:
return openFiles(_that);case Command_OpenUris() when openUris != null:
return openUris(_that);case Command_PlayFolder() when playFolder != null:
return playFolder(_that);case Command_BrowseFolder() when browseFolder != null:
return browseFolder(_that);case Command_ExportPlaylist() when exportPlaylist != null:
return exportPlaylist(_that);case Command_ImportPlaylist() when importPlaylist != null:
return importPlaylist(_that);case Command_AddStation() when addStation != null:
return addStation(_that);case Command_RemoveStation() when removeStation != null:
return removeStation(_that);case Command_UndoQueue() when undoQueue != null:
return undoQueue(_that);case Command_RedoQueue() when redoQueue != null:
return redoQueue(_that);case Command_SaveTags() when saveTags != null:
return saveTags(_that);case Command_ConvertAudio() when convertAudio != null:
return convertAudio(_that);case Command_CopyQueue() when copyQueue != null:
return copyQueue(_that);case Command_Cancel() when cancel != null:
return cancel(_that);case Command_SearchRadio() when searchRadio != null:
return searchRadio(_that);case Command_FetchLyrics() when fetchLyrics != null:
return fetchLyrics(_that);case Command_Rate() when rate != null:
return rate(_that);case Command_DismissError() when dismissError != null:
return dismissError(_that);case _:
  return null;

}
}
/// A variant of `when` that fallback to an `orElse` callback.
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return orElse();
/// }
/// ```

@optionalTypeArgs TResult maybeWhen<TResult extends Object?>({TResult Function()?  playPause,TResult Function()?  stop,TResult Function()?  next,TResult Function()?  previous,TResult Function()?  stopAfterCurrent,TResult Function( int seconds)?  seek,TResult Function( int volume)?  setVolume,TResult Function( LibraryQuery query,  int index)?  playLibrary,TResult Function( List<String> urls)?  enqueue,TResult Function( int index)?  playQueue,TResult Function( int index)?  removeQueue,TResult Function()?  clearQueue,TResult Function( int mode)?  setRepeat,TResult Function( int mode)?  setShuffle,TResult Function( int band,  double gain)?  setEqualizer,TResult Function( String theme)?  setTheme,TResult Function( int width,  int height)?  saveWindowSize,TResult Function( String path)?  addFolder,TResult Function( int id)?  removeFolder,TResult Function()?  rescan,TResult Function( int id)?  loadPlaylist,TResult Function( String name)?  savePlaylist,TResult Function( int id)?  deletePlaylist,TResult Function( List<String> paths)?  openFiles,TResult Function( List<String> uris)?  openUris,TResult Function( String path)?  playFolder,TResult Function( String path)?  browseFolder,TResult Function( String path)?  exportPlaylist,TResult Function( String path)?  importPlaylist,TResult Function( String name,  String url)?  addStation,TResult Function( int index)?  removeStation,TResult Function()?  undoQueue,TResult Function()?  redoQueue,TResult Function( TagEdit edit)?  saveTags,TResult Function( String source,  String format,  String destination)?  convertAudio,TResult Function( String destination)?  copyQueue,TResult Function()?  cancel,TResult Function( String text)?  searchRadio,TResult Function()?  fetchLyrics,TResult Function( String url,  double rating)?  rate,TResult Function()?  dismissError,required TResult orElse(),}) {final _that = this;
switch (_that) {
case Command_PlayPause() when playPause != null:
return playPause();case Command_Stop() when stop != null:
return stop();case Command_Next() when next != null:
return next();case Command_Previous() when previous != null:
return previous();case Command_StopAfterCurrent() when stopAfterCurrent != null:
return stopAfterCurrent();case Command_Seek() when seek != null:
return seek(_that.seconds);case Command_SetVolume() when setVolume != null:
return setVolume(_that.volume);case Command_PlayLibrary() when playLibrary != null:
return playLibrary(_that.query,_that.index);case Command_Enqueue() when enqueue != null:
return enqueue(_that.urls);case Command_PlayQueue() when playQueue != null:
return playQueue(_that.index);case Command_RemoveQueue() when removeQueue != null:
return removeQueue(_that.index);case Command_ClearQueue() when clearQueue != null:
return clearQueue();case Command_SetRepeat() when setRepeat != null:
return setRepeat(_that.mode);case Command_SetShuffle() when setShuffle != null:
return setShuffle(_that.mode);case Command_SetEqualizer() when setEqualizer != null:
return setEqualizer(_that.band,_that.gain);case Command_SetTheme() when setTheme != null:
return setTheme(_that.theme);case Command_SaveWindowSize() when saveWindowSize != null:
return saveWindowSize(_that.width,_that.height);case Command_AddFolder() when addFolder != null:
return addFolder(_that.path);case Command_RemoveFolder() when removeFolder != null:
return removeFolder(_that.id);case Command_Rescan() when rescan != null:
return rescan();case Command_LoadPlaylist() when loadPlaylist != null:
return loadPlaylist(_that.id);case Command_SavePlaylist() when savePlaylist != null:
return savePlaylist(_that.name);case Command_DeletePlaylist() when deletePlaylist != null:
return deletePlaylist(_that.id);case Command_OpenFiles() when openFiles != null:
return openFiles(_that.paths);case Command_OpenUris() when openUris != null:
return openUris(_that.uris);case Command_PlayFolder() when playFolder != null:
return playFolder(_that.path);case Command_BrowseFolder() when browseFolder != null:
return browseFolder(_that.path);case Command_ExportPlaylist() when exportPlaylist != null:
return exportPlaylist(_that.path);case Command_ImportPlaylist() when importPlaylist != null:
return importPlaylist(_that.path);case Command_AddStation() when addStation != null:
return addStation(_that.name,_that.url);case Command_RemoveStation() when removeStation != null:
return removeStation(_that.index);case Command_UndoQueue() when undoQueue != null:
return undoQueue();case Command_RedoQueue() when redoQueue != null:
return redoQueue();case Command_SaveTags() when saveTags != null:
return saveTags(_that.edit);case Command_ConvertAudio() when convertAudio != null:
return convertAudio(_that.source,_that.format,_that.destination);case Command_CopyQueue() when copyQueue != null:
return copyQueue(_that.destination);case Command_Cancel() when cancel != null:
return cancel();case Command_SearchRadio() when searchRadio != null:
return searchRadio(_that.text);case Command_FetchLyrics() when fetchLyrics != null:
return fetchLyrics();case Command_Rate() when rate != null:
return rate(_that.url,_that.rating);case Command_DismissError() when dismissError != null:
return dismissError();case _:
  return orElse();

}
}
/// A `switch`-like method, using callbacks.
///
/// As opposed to `map`, this offers destructuring.
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case Subclass2(:final field2):
///     return ...;
/// }
/// ```

@optionalTypeArgs TResult when<TResult extends Object?>({required TResult Function()  playPause,required TResult Function()  stop,required TResult Function()  next,required TResult Function()  previous,required TResult Function()  stopAfterCurrent,required TResult Function( int seconds)  seek,required TResult Function( int volume)  setVolume,required TResult Function( LibraryQuery query,  int index)  playLibrary,required TResult Function( List<String> urls)  enqueue,required TResult Function( int index)  playQueue,required TResult Function( int index)  removeQueue,required TResult Function()  clearQueue,required TResult Function( int mode)  setRepeat,required TResult Function( int mode)  setShuffle,required TResult Function( int band,  double gain)  setEqualizer,required TResult Function( String theme)  setTheme,required TResult Function( int width,  int height)  saveWindowSize,required TResult Function( String path)  addFolder,required TResult Function( int id)  removeFolder,required TResult Function()  rescan,required TResult Function( int id)  loadPlaylist,required TResult Function( String name)  savePlaylist,required TResult Function( int id)  deletePlaylist,required TResult Function( List<String> paths)  openFiles,required TResult Function( List<String> uris)  openUris,required TResult Function( String path)  playFolder,required TResult Function( String path)  browseFolder,required TResult Function( String path)  exportPlaylist,required TResult Function( String path)  importPlaylist,required TResult Function( String name,  String url)  addStation,required TResult Function( int index)  removeStation,required TResult Function()  undoQueue,required TResult Function()  redoQueue,required TResult Function( TagEdit edit)  saveTags,required TResult Function( String source,  String format,  String destination)  convertAudio,required TResult Function( String destination)  copyQueue,required TResult Function()  cancel,required TResult Function( String text)  searchRadio,required TResult Function()  fetchLyrics,required TResult Function( String url,  double rating)  rate,required TResult Function()  dismissError,}) {final _that = this;
switch (_that) {
case Command_PlayPause():
return playPause();case Command_Stop():
return stop();case Command_Next():
return next();case Command_Previous():
return previous();case Command_StopAfterCurrent():
return stopAfterCurrent();case Command_Seek():
return seek(_that.seconds);case Command_SetVolume():
return setVolume(_that.volume);case Command_PlayLibrary():
return playLibrary(_that.query,_that.index);case Command_Enqueue():
return enqueue(_that.urls);case Command_PlayQueue():
return playQueue(_that.index);case Command_RemoveQueue():
return removeQueue(_that.index);case Command_ClearQueue():
return clearQueue();case Command_SetRepeat():
return setRepeat(_that.mode);case Command_SetShuffle():
return setShuffle(_that.mode);case Command_SetEqualizer():
return setEqualizer(_that.band,_that.gain);case Command_SetTheme():
return setTheme(_that.theme);case Command_SaveWindowSize():
return saveWindowSize(_that.width,_that.height);case Command_AddFolder():
return addFolder(_that.path);case Command_RemoveFolder():
return removeFolder(_that.id);case Command_Rescan():
return rescan();case Command_LoadPlaylist():
return loadPlaylist(_that.id);case Command_SavePlaylist():
return savePlaylist(_that.name);case Command_DeletePlaylist():
return deletePlaylist(_that.id);case Command_OpenFiles():
return openFiles(_that.paths);case Command_OpenUris():
return openUris(_that.uris);case Command_PlayFolder():
return playFolder(_that.path);case Command_BrowseFolder():
return browseFolder(_that.path);case Command_ExportPlaylist():
return exportPlaylist(_that.path);case Command_ImportPlaylist():
return importPlaylist(_that.path);case Command_AddStation():
return addStation(_that.name,_that.url);case Command_RemoveStation():
return removeStation(_that.index);case Command_UndoQueue():
return undoQueue();case Command_RedoQueue():
return redoQueue();case Command_SaveTags():
return saveTags(_that.edit);case Command_ConvertAudio():
return convertAudio(_that.source,_that.format,_that.destination);case Command_CopyQueue():
return copyQueue(_that.destination);case Command_Cancel():
return cancel();case Command_SearchRadio():
return searchRadio(_that.text);case Command_FetchLyrics():
return fetchLyrics();case Command_Rate():
return rate(_that.url,_that.rating);case Command_DismissError():
return dismissError();}
}
/// A variant of `when` that fallback to returning `null`
///
/// It is equivalent to doing:
/// ```dart
/// switch (sealedClass) {
///   case Subclass(:final field):
///     return ...;
///   case _:
///     return null;
/// }
/// ```

@optionalTypeArgs TResult? whenOrNull<TResult extends Object?>({TResult? Function()?  playPause,TResult? Function()?  stop,TResult? Function()?  next,TResult? Function()?  previous,TResult? Function()?  stopAfterCurrent,TResult? Function( int seconds)?  seek,TResult? Function( int volume)?  setVolume,TResult? Function( LibraryQuery query,  int index)?  playLibrary,TResult? Function( List<String> urls)?  enqueue,TResult? Function( int index)?  playQueue,TResult? Function( int index)?  removeQueue,TResult? Function()?  clearQueue,TResult? Function( int mode)?  setRepeat,TResult? Function( int mode)?  setShuffle,TResult? Function( int band,  double gain)?  setEqualizer,TResult? Function( String theme)?  setTheme,TResult? Function( int width,  int height)?  saveWindowSize,TResult? Function( String path)?  addFolder,TResult? Function( int id)?  removeFolder,TResult? Function()?  rescan,TResult? Function( int id)?  loadPlaylist,TResult? Function( String name)?  savePlaylist,TResult? Function( int id)?  deletePlaylist,TResult? Function( List<String> paths)?  openFiles,TResult? Function( List<String> uris)?  openUris,TResult? Function( String path)?  playFolder,TResult? Function( String path)?  browseFolder,TResult? Function( String path)?  exportPlaylist,TResult? Function( String path)?  importPlaylist,TResult? Function( String name,  String url)?  addStation,TResult? Function( int index)?  removeStation,TResult? Function()?  undoQueue,TResult? Function()?  redoQueue,TResult? Function( TagEdit edit)?  saveTags,TResult? Function( String source,  String format,  String destination)?  convertAudio,TResult? Function( String destination)?  copyQueue,TResult? Function()?  cancel,TResult? Function( String text)?  searchRadio,TResult? Function()?  fetchLyrics,TResult? Function( String url,  double rating)?  rate,TResult? Function()?  dismissError,}) {final _that = this;
switch (_that) {
case Command_PlayPause() when playPause != null:
return playPause();case Command_Stop() when stop != null:
return stop();case Command_Next() when next != null:
return next();case Command_Previous() when previous != null:
return previous();case Command_StopAfterCurrent() when stopAfterCurrent != null:
return stopAfterCurrent();case Command_Seek() when seek != null:
return seek(_that.seconds);case Command_SetVolume() when setVolume != null:
return setVolume(_that.volume);case Command_PlayLibrary() when playLibrary != null:
return playLibrary(_that.query,_that.index);case Command_Enqueue() when enqueue != null:
return enqueue(_that.urls);case Command_PlayQueue() when playQueue != null:
return playQueue(_that.index);case Command_RemoveQueue() when removeQueue != null:
return removeQueue(_that.index);case Command_ClearQueue() when clearQueue != null:
return clearQueue();case Command_SetRepeat() when setRepeat != null:
return setRepeat(_that.mode);case Command_SetShuffle() when setShuffle != null:
return setShuffle(_that.mode);case Command_SetEqualizer() when setEqualizer != null:
return setEqualizer(_that.band,_that.gain);case Command_SetTheme() when setTheme != null:
return setTheme(_that.theme);case Command_SaveWindowSize() when saveWindowSize != null:
return saveWindowSize(_that.width,_that.height);case Command_AddFolder() when addFolder != null:
return addFolder(_that.path);case Command_RemoveFolder() when removeFolder != null:
return removeFolder(_that.id);case Command_Rescan() when rescan != null:
return rescan();case Command_LoadPlaylist() when loadPlaylist != null:
return loadPlaylist(_that.id);case Command_SavePlaylist() when savePlaylist != null:
return savePlaylist(_that.name);case Command_DeletePlaylist() when deletePlaylist != null:
return deletePlaylist(_that.id);case Command_OpenFiles() when openFiles != null:
return openFiles(_that.paths);case Command_OpenUris() when openUris != null:
return openUris(_that.uris);case Command_PlayFolder() when playFolder != null:
return playFolder(_that.path);case Command_BrowseFolder() when browseFolder != null:
return browseFolder(_that.path);case Command_ExportPlaylist() when exportPlaylist != null:
return exportPlaylist(_that.path);case Command_ImportPlaylist() when importPlaylist != null:
return importPlaylist(_that.path);case Command_AddStation() when addStation != null:
return addStation(_that.name,_that.url);case Command_RemoveStation() when removeStation != null:
return removeStation(_that.index);case Command_UndoQueue() when undoQueue != null:
return undoQueue();case Command_RedoQueue() when redoQueue != null:
return redoQueue();case Command_SaveTags() when saveTags != null:
return saveTags(_that.edit);case Command_ConvertAudio() when convertAudio != null:
return convertAudio(_that.source,_that.format,_that.destination);case Command_CopyQueue() when copyQueue != null:
return copyQueue(_that.destination);case Command_Cancel() when cancel != null:
return cancel();case Command_SearchRadio() when searchRadio != null:
return searchRadio(_that.text);case Command_FetchLyrics() when fetchLyrics != null:
return fetchLyrics();case Command_Rate() when rate != null:
return rate(_that.url,_that.rating);case Command_DismissError() when dismissError != null:
return dismissError();case _:
  return null;

}
}

}

/// @nodoc


class Command_PlayPause extends Command {
  const Command_PlayPause(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_PlayPause);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.playPause()';
}


}




/// @nodoc


class Command_Stop extends Command {
  const Command_Stop(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Stop);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.stop()';
}


}




/// @nodoc


class Command_Next extends Command {
  const Command_Next(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Next);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.next()';
}


}




/// @nodoc


class Command_Previous extends Command {
  const Command_Previous(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Previous);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.previous()';
}


}




/// @nodoc


class Command_StopAfterCurrent extends Command {
  const Command_StopAfterCurrent(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_StopAfterCurrent);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.stopAfterCurrent()';
}


}




/// @nodoc


class Command_Seek extends Command {
  const Command_Seek({required this.seconds}): super._();
  

 final  int seconds;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SeekCopyWith<Command_Seek> get copyWith => _$Command_SeekCopyWithImpl<Command_Seek>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Seek&&(identical(other.seconds, seconds) || other.seconds == seconds));
}


@override
int get hashCode {
    return Object.hash(runtimeType,seconds);
}

@override
String toString() {
    return 'Command.seek(seconds: $seconds)';
}


}

/// @nodoc
abstract mixin class $Command_SeekCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SeekCopyWith(Command_Seek value, $Res Function(Command_Seek) _then) = _$Command_SeekCopyWithImpl;
@useResult
$Res call({
 int seconds
});




}
/// @nodoc
class _$Command_SeekCopyWithImpl<$Res>
    implements $Command_SeekCopyWith<$Res> {
  _$Command_SeekCopyWithImpl(this._self, this._then);

  final Command_Seek _self;
  final $Res Function(Command_Seek) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? seconds = null,}) {
  return _then(Command_Seek(
seconds: null == seconds ? _self.seconds : seconds // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_SetVolume extends Command {
  const Command_SetVolume({required this.volume}): super._();
  

 final  int volume;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SetVolumeCopyWith<Command_SetVolume> get copyWith => _$Command_SetVolumeCopyWithImpl<Command_SetVolume>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SetVolume&&(identical(other.volume, volume) || other.volume == volume));
}


@override
int get hashCode {
    return Object.hash(runtimeType,volume);
}

@override
String toString() {
    return 'Command.setVolume(volume: $volume)';
}


}

/// @nodoc
abstract mixin class $Command_SetVolumeCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SetVolumeCopyWith(Command_SetVolume value, $Res Function(Command_SetVolume) _then) = _$Command_SetVolumeCopyWithImpl;
@useResult
$Res call({
 int volume
});




}
/// @nodoc
class _$Command_SetVolumeCopyWithImpl<$Res>
    implements $Command_SetVolumeCopyWith<$Res> {
  _$Command_SetVolumeCopyWithImpl(this._self, this._then);

  final Command_SetVolume _self;
  final $Res Function(Command_SetVolume) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? volume = null,}) {
  return _then(Command_SetVolume(
volume: null == volume ? _self.volume : volume // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_PlayLibrary extends Command {
  const Command_PlayLibrary({required this.query, required this.index}): super._();
  

 final  LibraryQuery query;
 final  int index;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_PlayLibraryCopyWith<Command_PlayLibrary> get copyWith => _$Command_PlayLibraryCopyWithImpl<Command_PlayLibrary>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_PlayLibrary&&(identical(other.query, query) || other.query == query)&&(identical(other.index, index) || other.index == index));
}


@override
int get hashCode {
    return Object.hash(runtimeType,query,index);
}

@override
String toString() {
    return 'Command.playLibrary(query: $query, index: $index)';
}


}

/// @nodoc
abstract mixin class $Command_PlayLibraryCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_PlayLibraryCopyWith(Command_PlayLibrary value, $Res Function(Command_PlayLibrary) _then) = _$Command_PlayLibraryCopyWithImpl;
@useResult
$Res call({
 LibraryQuery query, int index
});




}
/// @nodoc
class _$Command_PlayLibraryCopyWithImpl<$Res>
    implements $Command_PlayLibraryCopyWith<$Res> {
  _$Command_PlayLibraryCopyWithImpl(this._self, this._then);

  final Command_PlayLibrary _self;
  final $Res Function(Command_PlayLibrary) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? query = null,Object? index = null,}) {
  return _then(Command_PlayLibrary(
query: null == query ? _self.query : query // ignore: cast_nullable_to_non_nullable
as LibraryQuery,index: null == index ? _self.index : index // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_Enqueue extends Command {
  const Command_Enqueue({required  List<String> urls}): _urls = urls,super._();
  

 final  List<String> _urls;
 List<String> get urls {
  if (_urls is EqualUnmodifiableListView) return _urls;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_urls);
}


/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_EnqueueCopyWith<Command_Enqueue> get copyWith => _$Command_EnqueueCopyWithImpl<Command_Enqueue>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Enqueue&&const DeepCollectionEquality().equals(other.urls, _urls));
}


@override
int get hashCode {
    return Object.hash(runtimeType,const DeepCollectionEquality().hash(_urls));
}

@override
String toString() {
    return 'Command.enqueue(urls: $urls)';
}


}

/// @nodoc
abstract mixin class $Command_EnqueueCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_EnqueueCopyWith(Command_Enqueue value, $Res Function(Command_Enqueue) _then) = _$Command_EnqueueCopyWithImpl;
@useResult
$Res call({
 List<String> urls
});




}
/// @nodoc
class _$Command_EnqueueCopyWithImpl<$Res>
    implements $Command_EnqueueCopyWith<$Res> {
  _$Command_EnqueueCopyWithImpl(this._self, this._then);

  final Command_Enqueue _self;
  final $Res Function(Command_Enqueue) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? urls = null,}) {
  return _then(Command_Enqueue(
urls: null == urls ? _self._urls : urls // ignore: cast_nullable_to_non_nullable
as List<String>,
  ));
}


}

/// @nodoc


class Command_PlayQueue extends Command {
  const Command_PlayQueue({required this.index}): super._();
  

 final  int index;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_PlayQueueCopyWith<Command_PlayQueue> get copyWith => _$Command_PlayQueueCopyWithImpl<Command_PlayQueue>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_PlayQueue&&(identical(other.index, index) || other.index == index));
}


@override
int get hashCode {
    return Object.hash(runtimeType,index);
}

@override
String toString() {
    return 'Command.playQueue(index: $index)';
}


}

/// @nodoc
abstract mixin class $Command_PlayQueueCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_PlayQueueCopyWith(Command_PlayQueue value, $Res Function(Command_PlayQueue) _then) = _$Command_PlayQueueCopyWithImpl;
@useResult
$Res call({
 int index
});




}
/// @nodoc
class _$Command_PlayQueueCopyWithImpl<$Res>
    implements $Command_PlayQueueCopyWith<$Res> {
  _$Command_PlayQueueCopyWithImpl(this._self, this._then);

  final Command_PlayQueue _self;
  final $Res Function(Command_PlayQueue) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? index = null,}) {
  return _then(Command_PlayQueue(
index: null == index ? _self.index : index // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_RemoveQueue extends Command {
  const Command_RemoveQueue({required this.index}): super._();
  

 final  int index;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_RemoveQueueCopyWith<Command_RemoveQueue> get copyWith => _$Command_RemoveQueueCopyWithImpl<Command_RemoveQueue>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_RemoveQueue&&(identical(other.index, index) || other.index == index));
}


@override
int get hashCode {
    return Object.hash(runtimeType,index);
}

@override
String toString() {
    return 'Command.removeQueue(index: $index)';
}


}

/// @nodoc
abstract mixin class $Command_RemoveQueueCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_RemoveQueueCopyWith(Command_RemoveQueue value, $Res Function(Command_RemoveQueue) _then) = _$Command_RemoveQueueCopyWithImpl;
@useResult
$Res call({
 int index
});




}
/// @nodoc
class _$Command_RemoveQueueCopyWithImpl<$Res>
    implements $Command_RemoveQueueCopyWith<$Res> {
  _$Command_RemoveQueueCopyWithImpl(this._self, this._then);

  final Command_RemoveQueue _self;
  final $Res Function(Command_RemoveQueue) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? index = null,}) {
  return _then(Command_RemoveQueue(
index: null == index ? _self.index : index // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_ClearQueue extends Command {
  const Command_ClearQueue(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_ClearQueue);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.clearQueue()';
}


}




/// @nodoc


class Command_SetRepeat extends Command {
  const Command_SetRepeat({required this.mode}): super._();
  

 final  int mode;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SetRepeatCopyWith<Command_SetRepeat> get copyWith => _$Command_SetRepeatCopyWithImpl<Command_SetRepeat>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SetRepeat&&(identical(other.mode, mode) || other.mode == mode));
}


@override
int get hashCode {
    return Object.hash(runtimeType,mode);
}

@override
String toString() {
    return 'Command.setRepeat(mode: $mode)';
}


}

/// @nodoc
abstract mixin class $Command_SetRepeatCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SetRepeatCopyWith(Command_SetRepeat value, $Res Function(Command_SetRepeat) _then) = _$Command_SetRepeatCopyWithImpl;
@useResult
$Res call({
 int mode
});




}
/// @nodoc
class _$Command_SetRepeatCopyWithImpl<$Res>
    implements $Command_SetRepeatCopyWith<$Res> {
  _$Command_SetRepeatCopyWithImpl(this._self, this._then);

  final Command_SetRepeat _self;
  final $Res Function(Command_SetRepeat) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? mode = null,}) {
  return _then(Command_SetRepeat(
mode: null == mode ? _self.mode : mode // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_SetShuffle extends Command {
  const Command_SetShuffle({required this.mode}): super._();
  

 final  int mode;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SetShuffleCopyWith<Command_SetShuffle> get copyWith => _$Command_SetShuffleCopyWithImpl<Command_SetShuffle>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SetShuffle&&(identical(other.mode, mode) || other.mode == mode));
}


@override
int get hashCode {
    return Object.hash(runtimeType,mode);
}

@override
String toString() {
    return 'Command.setShuffle(mode: $mode)';
}


}

/// @nodoc
abstract mixin class $Command_SetShuffleCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SetShuffleCopyWith(Command_SetShuffle value, $Res Function(Command_SetShuffle) _then) = _$Command_SetShuffleCopyWithImpl;
@useResult
$Res call({
 int mode
});




}
/// @nodoc
class _$Command_SetShuffleCopyWithImpl<$Res>
    implements $Command_SetShuffleCopyWith<$Res> {
  _$Command_SetShuffleCopyWithImpl(this._self, this._then);

  final Command_SetShuffle _self;
  final $Res Function(Command_SetShuffle) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? mode = null,}) {
  return _then(Command_SetShuffle(
mode: null == mode ? _self.mode : mode // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_SetEqualizer extends Command {
  const Command_SetEqualizer({required this.band, required this.gain}): super._();
  

 final  int band;
 final  double gain;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SetEqualizerCopyWith<Command_SetEqualizer> get copyWith => _$Command_SetEqualizerCopyWithImpl<Command_SetEqualizer>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SetEqualizer&&(identical(other.band, band) || other.band == band)&&(identical(other.gain, gain) || other.gain == gain));
}


@override
int get hashCode {
    return Object.hash(runtimeType,band,gain);
}

@override
String toString() {
    return 'Command.setEqualizer(band: $band, gain: $gain)';
}


}

/// @nodoc
abstract mixin class $Command_SetEqualizerCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SetEqualizerCopyWith(Command_SetEqualizer value, $Res Function(Command_SetEqualizer) _then) = _$Command_SetEqualizerCopyWithImpl;
@useResult
$Res call({
 int band, double gain
});




}
/// @nodoc
class _$Command_SetEqualizerCopyWithImpl<$Res>
    implements $Command_SetEqualizerCopyWith<$Res> {
  _$Command_SetEqualizerCopyWithImpl(this._self, this._then);

  final Command_SetEqualizer _self;
  final $Res Function(Command_SetEqualizer) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? band = null,Object? gain = null,}) {
  return _then(Command_SetEqualizer(
band: null == band ? _self.band : band // ignore: cast_nullable_to_non_nullable
as int,gain: null == gain ? _self.gain : gain // ignore: cast_nullable_to_non_nullable
as double,
  ));
}


}

/// @nodoc


class Command_SetTheme extends Command {
  const Command_SetTheme({required this.theme}): super._();
  

 final  String theme;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SetThemeCopyWith<Command_SetTheme> get copyWith => _$Command_SetThemeCopyWithImpl<Command_SetTheme>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SetTheme&&(identical(other.theme, theme) || other.theme == theme));
}


@override
int get hashCode {
    return Object.hash(runtimeType,theme);
}

@override
String toString() {
    return 'Command.setTheme(theme: $theme)';
}


}

/// @nodoc
abstract mixin class $Command_SetThemeCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SetThemeCopyWith(Command_SetTheme value, $Res Function(Command_SetTheme) _then) = _$Command_SetThemeCopyWithImpl;
@useResult
$Res call({
 String theme
});




}
/// @nodoc
class _$Command_SetThemeCopyWithImpl<$Res>
    implements $Command_SetThemeCopyWith<$Res> {
  _$Command_SetThemeCopyWithImpl(this._self, this._then);

  final Command_SetTheme _self;
  final $Res Function(Command_SetTheme) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? theme = null,}) {
  return _then(Command_SetTheme(
theme: null == theme ? _self.theme : theme // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_SaveWindowSize extends Command {
  const Command_SaveWindowSize({required this.width, required this.height}): super._();
  

 final  int width;
 final  int height;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SaveWindowSizeCopyWith<Command_SaveWindowSize> get copyWith => _$Command_SaveWindowSizeCopyWithImpl<Command_SaveWindowSize>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SaveWindowSize&&(identical(other.width, width) || other.width == width)&&(identical(other.height, height) || other.height == height));
}


@override
int get hashCode {
    return Object.hash(runtimeType,width,height);
}

@override
String toString() {
    return 'Command.saveWindowSize(width: $width, height: $height)';
}


}

/// @nodoc
abstract mixin class $Command_SaveWindowSizeCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SaveWindowSizeCopyWith(Command_SaveWindowSize value, $Res Function(Command_SaveWindowSize) _then) = _$Command_SaveWindowSizeCopyWithImpl;
@useResult
$Res call({
 int width, int height
});




}
/// @nodoc
class _$Command_SaveWindowSizeCopyWithImpl<$Res>
    implements $Command_SaveWindowSizeCopyWith<$Res> {
  _$Command_SaveWindowSizeCopyWithImpl(this._self, this._then);

  final Command_SaveWindowSize _self;
  final $Res Function(Command_SaveWindowSize) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? width = null,Object? height = null,}) {
  return _then(Command_SaveWindowSize(
width: null == width ? _self.width : width // ignore: cast_nullable_to_non_nullable
as int,height: null == height ? _self.height : height // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_AddFolder extends Command {
  const Command_AddFolder({required this.path}): super._();
  

 final  String path;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_AddFolderCopyWith<Command_AddFolder> get copyWith => _$Command_AddFolderCopyWithImpl<Command_AddFolder>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_AddFolder&&(identical(other.path, path) || other.path == path));
}


@override
int get hashCode {
    return Object.hash(runtimeType,path);
}

@override
String toString() {
    return 'Command.addFolder(path: $path)';
}


}

/// @nodoc
abstract mixin class $Command_AddFolderCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_AddFolderCopyWith(Command_AddFolder value, $Res Function(Command_AddFolder) _then) = _$Command_AddFolderCopyWithImpl;
@useResult
$Res call({
 String path
});




}
/// @nodoc
class _$Command_AddFolderCopyWithImpl<$Res>
    implements $Command_AddFolderCopyWith<$Res> {
  _$Command_AddFolderCopyWithImpl(this._self, this._then);

  final Command_AddFolder _self;
  final $Res Function(Command_AddFolder) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? path = null,}) {
  return _then(Command_AddFolder(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_RemoveFolder extends Command {
  const Command_RemoveFolder({required this.id}): super._();
  

 final  int id;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_RemoveFolderCopyWith<Command_RemoveFolder> get copyWith => _$Command_RemoveFolderCopyWithImpl<Command_RemoveFolder>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_RemoveFolder&&(identical(other.id, id) || other.id == id));
}


@override
int get hashCode {
    return Object.hash(runtimeType,id);
}

@override
String toString() {
    return 'Command.removeFolder(id: $id)';
}


}

/// @nodoc
abstract mixin class $Command_RemoveFolderCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_RemoveFolderCopyWith(Command_RemoveFolder value, $Res Function(Command_RemoveFolder) _then) = _$Command_RemoveFolderCopyWithImpl;
@useResult
$Res call({
 int id
});




}
/// @nodoc
class _$Command_RemoveFolderCopyWithImpl<$Res>
    implements $Command_RemoveFolderCopyWith<$Res> {
  _$Command_RemoveFolderCopyWithImpl(this._self, this._then);

  final Command_RemoveFolder _self;
  final $Res Function(Command_RemoveFolder) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? id = null,}) {
  return _then(Command_RemoveFolder(
id: null == id ? _self.id : id // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_Rescan extends Command {
  const Command_Rescan(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Rescan);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.rescan()';
}


}




/// @nodoc


class Command_LoadPlaylist extends Command {
  const Command_LoadPlaylist({required this.id}): super._();
  

 final  int id;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_LoadPlaylistCopyWith<Command_LoadPlaylist> get copyWith => _$Command_LoadPlaylistCopyWithImpl<Command_LoadPlaylist>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_LoadPlaylist&&(identical(other.id, id) || other.id == id));
}


@override
int get hashCode {
    return Object.hash(runtimeType,id);
}

@override
String toString() {
    return 'Command.loadPlaylist(id: $id)';
}


}

/// @nodoc
abstract mixin class $Command_LoadPlaylistCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_LoadPlaylistCopyWith(Command_LoadPlaylist value, $Res Function(Command_LoadPlaylist) _then) = _$Command_LoadPlaylistCopyWithImpl;
@useResult
$Res call({
 int id
});




}
/// @nodoc
class _$Command_LoadPlaylistCopyWithImpl<$Res>
    implements $Command_LoadPlaylistCopyWith<$Res> {
  _$Command_LoadPlaylistCopyWithImpl(this._self, this._then);

  final Command_LoadPlaylist _self;
  final $Res Function(Command_LoadPlaylist) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? id = null,}) {
  return _then(Command_LoadPlaylist(
id: null == id ? _self.id : id // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_SavePlaylist extends Command {
  const Command_SavePlaylist({required this.name}): super._();
  

 final  String name;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SavePlaylistCopyWith<Command_SavePlaylist> get copyWith => _$Command_SavePlaylistCopyWithImpl<Command_SavePlaylist>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SavePlaylist&&(identical(other.name, name) || other.name == name));
}


@override
int get hashCode {
    return Object.hash(runtimeType,name);
}

@override
String toString() {
    return 'Command.savePlaylist(name: $name)';
}


}

/// @nodoc
abstract mixin class $Command_SavePlaylistCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SavePlaylistCopyWith(Command_SavePlaylist value, $Res Function(Command_SavePlaylist) _then) = _$Command_SavePlaylistCopyWithImpl;
@useResult
$Res call({
 String name
});




}
/// @nodoc
class _$Command_SavePlaylistCopyWithImpl<$Res>
    implements $Command_SavePlaylistCopyWith<$Res> {
  _$Command_SavePlaylistCopyWithImpl(this._self, this._then);

  final Command_SavePlaylist _self;
  final $Res Function(Command_SavePlaylist) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? name = null,}) {
  return _then(Command_SavePlaylist(
name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_DeletePlaylist extends Command {
  const Command_DeletePlaylist({required this.id}): super._();
  

 final  int id;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_DeletePlaylistCopyWith<Command_DeletePlaylist> get copyWith => _$Command_DeletePlaylistCopyWithImpl<Command_DeletePlaylist>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_DeletePlaylist&&(identical(other.id, id) || other.id == id));
}


@override
int get hashCode {
    return Object.hash(runtimeType,id);
}

@override
String toString() {
    return 'Command.deletePlaylist(id: $id)';
}


}

/// @nodoc
abstract mixin class $Command_DeletePlaylistCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_DeletePlaylistCopyWith(Command_DeletePlaylist value, $Res Function(Command_DeletePlaylist) _then) = _$Command_DeletePlaylistCopyWithImpl;
@useResult
$Res call({
 int id
});




}
/// @nodoc
class _$Command_DeletePlaylistCopyWithImpl<$Res>
    implements $Command_DeletePlaylistCopyWith<$Res> {
  _$Command_DeletePlaylistCopyWithImpl(this._self, this._then);

  final Command_DeletePlaylist _self;
  final $Res Function(Command_DeletePlaylist) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? id = null,}) {
  return _then(Command_DeletePlaylist(
id: null == id ? _self.id : id // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_OpenFiles extends Command {
  const Command_OpenFiles({required  List<String> paths}): _paths = paths,super._();
  

 final  List<String> _paths;
 List<String> get paths {
  if (_paths is EqualUnmodifiableListView) return _paths;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_paths);
}


/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_OpenFilesCopyWith<Command_OpenFiles> get copyWith => _$Command_OpenFilesCopyWithImpl<Command_OpenFiles>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_OpenFiles&&const DeepCollectionEquality().equals(other.paths, _paths));
}


@override
int get hashCode {
    return Object.hash(runtimeType,const DeepCollectionEquality().hash(_paths));
}

@override
String toString() {
    return 'Command.openFiles(paths: $paths)';
}


}

/// @nodoc
abstract mixin class $Command_OpenFilesCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_OpenFilesCopyWith(Command_OpenFiles value, $Res Function(Command_OpenFiles) _then) = _$Command_OpenFilesCopyWithImpl;
@useResult
$Res call({
 List<String> paths
});




}
/// @nodoc
class _$Command_OpenFilesCopyWithImpl<$Res>
    implements $Command_OpenFilesCopyWith<$Res> {
  _$Command_OpenFilesCopyWithImpl(this._self, this._then);

  final Command_OpenFiles _self;
  final $Res Function(Command_OpenFiles) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? paths = null,}) {
  return _then(Command_OpenFiles(
paths: null == paths ? _self._paths : paths // ignore: cast_nullable_to_non_nullable
as List<String>,
  ));
}


}

/// @nodoc


class Command_OpenUris extends Command {
  const Command_OpenUris({required  List<String> uris}): _uris = uris,super._();
  

 final  List<String> _uris;
 List<String> get uris {
  if (_uris is EqualUnmodifiableListView) return _uris;
  // ignore: implicit_dynamic_type
  return EqualUnmodifiableListView(_uris);
}


/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_OpenUrisCopyWith<Command_OpenUris> get copyWith => _$Command_OpenUrisCopyWithImpl<Command_OpenUris>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_OpenUris&&const DeepCollectionEquality().equals(other.uris, _uris));
}


@override
int get hashCode {
    return Object.hash(runtimeType,const DeepCollectionEquality().hash(_uris));
}

@override
String toString() {
    return 'Command.openUris(uris: $uris)';
}


}

/// @nodoc
abstract mixin class $Command_OpenUrisCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_OpenUrisCopyWith(Command_OpenUris value, $Res Function(Command_OpenUris) _then) = _$Command_OpenUrisCopyWithImpl;
@useResult
$Res call({
 List<String> uris
});




}
/// @nodoc
class _$Command_OpenUrisCopyWithImpl<$Res>
    implements $Command_OpenUrisCopyWith<$Res> {
  _$Command_OpenUrisCopyWithImpl(this._self, this._then);

  final Command_OpenUris _self;
  final $Res Function(Command_OpenUris) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? uris = null,}) {
  return _then(Command_OpenUris(
uris: null == uris ? _self._uris : uris // ignore: cast_nullable_to_non_nullable
as List<String>,
  ));
}


}

/// @nodoc


class Command_PlayFolder extends Command {
  const Command_PlayFolder({required this.path}): super._();
  

 final  String path;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_PlayFolderCopyWith<Command_PlayFolder> get copyWith => _$Command_PlayFolderCopyWithImpl<Command_PlayFolder>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_PlayFolder&&(identical(other.path, path) || other.path == path));
}


@override
int get hashCode {
    return Object.hash(runtimeType,path);
}

@override
String toString() {
    return 'Command.playFolder(path: $path)';
}


}

/// @nodoc
abstract mixin class $Command_PlayFolderCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_PlayFolderCopyWith(Command_PlayFolder value, $Res Function(Command_PlayFolder) _then) = _$Command_PlayFolderCopyWithImpl;
@useResult
$Res call({
 String path
});




}
/// @nodoc
class _$Command_PlayFolderCopyWithImpl<$Res>
    implements $Command_PlayFolderCopyWith<$Res> {
  _$Command_PlayFolderCopyWithImpl(this._self, this._then);

  final Command_PlayFolder _self;
  final $Res Function(Command_PlayFolder) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? path = null,}) {
  return _then(Command_PlayFolder(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_BrowseFolder extends Command {
  const Command_BrowseFolder({required this.path}): super._();
  

 final  String path;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_BrowseFolderCopyWith<Command_BrowseFolder> get copyWith => _$Command_BrowseFolderCopyWithImpl<Command_BrowseFolder>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_BrowseFolder&&(identical(other.path, path) || other.path == path));
}


@override
int get hashCode {
    return Object.hash(runtimeType,path);
}

@override
String toString() {
    return 'Command.browseFolder(path: $path)';
}


}

/// @nodoc
abstract mixin class $Command_BrowseFolderCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_BrowseFolderCopyWith(Command_BrowseFolder value, $Res Function(Command_BrowseFolder) _then) = _$Command_BrowseFolderCopyWithImpl;
@useResult
$Res call({
 String path
});




}
/// @nodoc
class _$Command_BrowseFolderCopyWithImpl<$Res>
    implements $Command_BrowseFolderCopyWith<$Res> {
  _$Command_BrowseFolderCopyWithImpl(this._self, this._then);

  final Command_BrowseFolder _self;
  final $Res Function(Command_BrowseFolder) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? path = null,}) {
  return _then(Command_BrowseFolder(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_ExportPlaylist extends Command {
  const Command_ExportPlaylist({required this.path}): super._();
  

 final  String path;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_ExportPlaylistCopyWith<Command_ExportPlaylist> get copyWith => _$Command_ExportPlaylistCopyWithImpl<Command_ExportPlaylist>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_ExportPlaylist&&(identical(other.path, path) || other.path == path));
}


@override
int get hashCode {
    return Object.hash(runtimeType,path);
}

@override
String toString() {
    return 'Command.exportPlaylist(path: $path)';
}


}

/// @nodoc
abstract mixin class $Command_ExportPlaylistCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_ExportPlaylistCopyWith(Command_ExportPlaylist value, $Res Function(Command_ExportPlaylist) _then) = _$Command_ExportPlaylistCopyWithImpl;
@useResult
$Res call({
 String path
});




}
/// @nodoc
class _$Command_ExportPlaylistCopyWithImpl<$Res>
    implements $Command_ExportPlaylistCopyWith<$Res> {
  _$Command_ExportPlaylistCopyWithImpl(this._self, this._then);

  final Command_ExportPlaylist _self;
  final $Res Function(Command_ExportPlaylist) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? path = null,}) {
  return _then(Command_ExportPlaylist(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_ImportPlaylist extends Command {
  const Command_ImportPlaylist({required this.path}): super._();
  

 final  String path;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_ImportPlaylistCopyWith<Command_ImportPlaylist> get copyWith => _$Command_ImportPlaylistCopyWithImpl<Command_ImportPlaylist>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_ImportPlaylist&&(identical(other.path, path) || other.path == path));
}


@override
int get hashCode {
    return Object.hash(runtimeType,path);
}

@override
String toString() {
    return 'Command.importPlaylist(path: $path)';
}


}

/// @nodoc
abstract mixin class $Command_ImportPlaylistCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_ImportPlaylistCopyWith(Command_ImportPlaylist value, $Res Function(Command_ImportPlaylist) _then) = _$Command_ImportPlaylistCopyWithImpl;
@useResult
$Res call({
 String path
});




}
/// @nodoc
class _$Command_ImportPlaylistCopyWithImpl<$Res>
    implements $Command_ImportPlaylistCopyWith<$Res> {
  _$Command_ImportPlaylistCopyWithImpl(this._self, this._then);

  final Command_ImportPlaylist _self;
  final $Res Function(Command_ImportPlaylist) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? path = null,}) {
  return _then(Command_ImportPlaylist(
path: null == path ? _self.path : path // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_AddStation extends Command {
  const Command_AddStation({required this.name, required this.url}): super._();
  

 final  String name;
 final  String url;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_AddStationCopyWith<Command_AddStation> get copyWith => _$Command_AddStationCopyWithImpl<Command_AddStation>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_AddStation&&(identical(other.name, name) || other.name == name)&&(identical(other.url, url) || other.url == url));
}


@override
int get hashCode {
    return Object.hash(runtimeType,name,url);
}

@override
String toString() {
    return 'Command.addStation(name: $name, url: $url)';
}


}

/// @nodoc
abstract mixin class $Command_AddStationCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_AddStationCopyWith(Command_AddStation value, $Res Function(Command_AddStation) _then) = _$Command_AddStationCopyWithImpl;
@useResult
$Res call({
 String name, String url
});




}
/// @nodoc
class _$Command_AddStationCopyWithImpl<$Res>
    implements $Command_AddStationCopyWith<$Res> {
  _$Command_AddStationCopyWithImpl(this._self, this._then);

  final Command_AddStation _self;
  final $Res Function(Command_AddStation) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? name = null,Object? url = null,}) {
  return _then(Command_AddStation(
name: null == name ? _self.name : name // ignore: cast_nullable_to_non_nullable
as String,url: null == url ? _self.url : url // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_RemoveStation extends Command {
  const Command_RemoveStation({required this.index}): super._();
  

 final  int index;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_RemoveStationCopyWith<Command_RemoveStation> get copyWith => _$Command_RemoveStationCopyWithImpl<Command_RemoveStation>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_RemoveStation&&(identical(other.index, index) || other.index == index));
}


@override
int get hashCode {
    return Object.hash(runtimeType,index);
}

@override
String toString() {
    return 'Command.removeStation(index: $index)';
}


}

/// @nodoc
abstract mixin class $Command_RemoveStationCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_RemoveStationCopyWith(Command_RemoveStation value, $Res Function(Command_RemoveStation) _then) = _$Command_RemoveStationCopyWithImpl;
@useResult
$Res call({
 int index
});




}
/// @nodoc
class _$Command_RemoveStationCopyWithImpl<$Res>
    implements $Command_RemoveStationCopyWith<$Res> {
  _$Command_RemoveStationCopyWithImpl(this._self, this._then);

  final Command_RemoveStation _self;
  final $Res Function(Command_RemoveStation) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? index = null,}) {
  return _then(Command_RemoveStation(
index: null == index ? _self.index : index // ignore: cast_nullable_to_non_nullable
as int,
  ));
}


}

/// @nodoc


class Command_UndoQueue extends Command {
  const Command_UndoQueue(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_UndoQueue);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.undoQueue()';
}


}




/// @nodoc


class Command_RedoQueue extends Command {
  const Command_RedoQueue(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_RedoQueue);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.redoQueue()';
}


}




/// @nodoc


class Command_SaveTags extends Command {
  const Command_SaveTags({required this.edit}): super._();
  

 final  TagEdit edit;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SaveTagsCopyWith<Command_SaveTags> get copyWith => _$Command_SaveTagsCopyWithImpl<Command_SaveTags>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SaveTags&&(identical(other.edit, edit) || other.edit == edit));
}


@override
int get hashCode {
    return Object.hash(runtimeType,edit);
}

@override
String toString() {
    return 'Command.saveTags(edit: $edit)';
}


}

/// @nodoc
abstract mixin class $Command_SaveTagsCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SaveTagsCopyWith(Command_SaveTags value, $Res Function(Command_SaveTags) _then) = _$Command_SaveTagsCopyWithImpl;
@useResult
$Res call({
 TagEdit edit
});




}
/// @nodoc
class _$Command_SaveTagsCopyWithImpl<$Res>
    implements $Command_SaveTagsCopyWith<$Res> {
  _$Command_SaveTagsCopyWithImpl(this._self, this._then);

  final Command_SaveTags _self;
  final $Res Function(Command_SaveTags) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? edit = null,}) {
  return _then(Command_SaveTags(
edit: null == edit ? _self.edit : edit // ignore: cast_nullable_to_non_nullable
as TagEdit,
  ));
}


}

/// @nodoc


class Command_ConvertAudio extends Command {
  const Command_ConvertAudio({required this.source, required this.format, required this.destination}): super._();
  

 final  String source;
 final  String format;
 final  String destination;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_ConvertAudioCopyWith<Command_ConvertAudio> get copyWith => _$Command_ConvertAudioCopyWithImpl<Command_ConvertAudio>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_ConvertAudio&&(identical(other.source, source) || other.source == source)&&(identical(other.format, format) || other.format == format)&&(identical(other.destination, destination) || other.destination == destination));
}


@override
int get hashCode {
    return Object.hash(runtimeType,source,format,destination);
}

@override
String toString() {
    return 'Command.convertAudio(source: $source, format: $format, destination: $destination)';
}


}

/// @nodoc
abstract mixin class $Command_ConvertAudioCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_ConvertAudioCopyWith(Command_ConvertAudio value, $Res Function(Command_ConvertAudio) _then) = _$Command_ConvertAudioCopyWithImpl;
@useResult
$Res call({
 String source, String format, String destination
});




}
/// @nodoc
class _$Command_ConvertAudioCopyWithImpl<$Res>
    implements $Command_ConvertAudioCopyWith<$Res> {
  _$Command_ConvertAudioCopyWithImpl(this._self, this._then);

  final Command_ConvertAudio _self;
  final $Res Function(Command_ConvertAudio) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? source = null,Object? format = null,Object? destination = null,}) {
  return _then(Command_ConvertAudio(
source: null == source ? _self.source : source // ignore: cast_nullable_to_non_nullable
as String,format: null == format ? _self.format : format // ignore: cast_nullable_to_non_nullable
as String,destination: null == destination ? _self.destination : destination // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_CopyQueue extends Command {
  const Command_CopyQueue({required this.destination}): super._();
  

 final  String destination;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_CopyQueueCopyWith<Command_CopyQueue> get copyWith => _$Command_CopyQueueCopyWithImpl<Command_CopyQueue>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_CopyQueue&&(identical(other.destination, destination) || other.destination == destination));
}


@override
int get hashCode {
    return Object.hash(runtimeType,destination);
}

@override
String toString() {
    return 'Command.copyQueue(destination: $destination)';
}


}

/// @nodoc
abstract mixin class $Command_CopyQueueCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_CopyQueueCopyWith(Command_CopyQueue value, $Res Function(Command_CopyQueue) _then) = _$Command_CopyQueueCopyWithImpl;
@useResult
$Res call({
 String destination
});




}
/// @nodoc
class _$Command_CopyQueueCopyWithImpl<$Res>
    implements $Command_CopyQueueCopyWith<$Res> {
  _$Command_CopyQueueCopyWithImpl(this._self, this._then);

  final Command_CopyQueue _self;
  final $Res Function(Command_CopyQueue) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? destination = null,}) {
  return _then(Command_CopyQueue(
destination: null == destination ? _self.destination : destination // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_Cancel extends Command {
  const Command_Cancel(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Cancel);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.cancel()';
}


}




/// @nodoc


class Command_SearchRadio extends Command {
  const Command_SearchRadio({required this.text}): super._();
  

 final  String text;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_SearchRadioCopyWith<Command_SearchRadio> get copyWith => _$Command_SearchRadioCopyWithImpl<Command_SearchRadio>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_SearchRadio&&(identical(other.text, text) || other.text == text));
}


@override
int get hashCode {
    return Object.hash(runtimeType,text);
}

@override
String toString() {
    return 'Command.searchRadio(text: $text)';
}


}

/// @nodoc
abstract mixin class $Command_SearchRadioCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_SearchRadioCopyWith(Command_SearchRadio value, $Res Function(Command_SearchRadio) _then) = _$Command_SearchRadioCopyWithImpl;
@useResult
$Res call({
 String text
});




}
/// @nodoc
class _$Command_SearchRadioCopyWithImpl<$Res>
    implements $Command_SearchRadioCopyWith<$Res> {
  _$Command_SearchRadioCopyWithImpl(this._self, this._then);

  final Command_SearchRadio _self;
  final $Res Function(Command_SearchRadio) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? text = null,}) {
  return _then(Command_SearchRadio(
text: null == text ? _self.text : text // ignore: cast_nullable_to_non_nullable
as String,
  ));
}


}

/// @nodoc


class Command_FetchLyrics extends Command {
  const Command_FetchLyrics(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_FetchLyrics);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.fetchLyrics()';
}


}




/// @nodoc


class Command_Rate extends Command {
  const Command_Rate({required this.url, required this.rating}): super._();
  

 final  String url;
 final  double rating;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@JsonKey(includeFromJson: false, includeToJson: false)
@pragma('vm:prefer-inline')
$Command_RateCopyWith<Command_Rate> get copyWith => _$Command_RateCopyWithImpl<Command_Rate>(this, _$identity);



@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_Rate&&(identical(other.url, url) || other.url == url)&&(identical(other.rating, rating) || other.rating == rating));
}


@override
int get hashCode {
    return Object.hash(runtimeType,url,rating);
}

@override
String toString() {
    return 'Command.rate(url: $url, rating: $rating)';
}


}

/// @nodoc
abstract mixin class $Command_RateCopyWith<$Res> implements $CommandCopyWith<$Res> {
  factory $Command_RateCopyWith(Command_Rate value, $Res Function(Command_Rate) _then) = _$Command_RateCopyWithImpl;
@useResult
$Res call({
 String url, double rating
});




}
/// @nodoc
class _$Command_RateCopyWithImpl<$Res>
    implements $Command_RateCopyWith<$Res> {
  _$Command_RateCopyWithImpl(this._self, this._then);

  final Command_Rate _self;
  final $Res Function(Command_Rate) _then;

/// Create a copy of Command
/// with the given fields replaced by the non-null parameter values.
@pragma('vm:prefer-inline') $Res call({Object? url = null,Object? rating = null,}) {
  return _then(Command_Rate(
url: null == url ? _self.url : url // ignore: cast_nullable_to_non_nullable
as String,rating: null == rating ? _self.rating : rating // ignore: cast_nullable_to_non_nullable
as double,
  ));
}


}

/// @nodoc


class Command_DismissError extends Command {
  const Command_DismissError(): super._();
  






@override
bool operator ==(Object other) {
    return identical(this, other) || (other.runtimeType == runtimeType&&other is Command_DismissError);
}


@override
int get hashCode => runtimeType.hashCode;

@override
String toString() {
    return 'Command.dismissError()';
}


}




// dart format on
