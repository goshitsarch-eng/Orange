# Orange 2.1.6 GUI and wiring audit

## Scope

Reviewed main-window action connections, settings load/apply/cancel, radio search/service lifecycle, file-browser navigation and tree roots, waveform controller wiring, release identity and build workflows. This is a targeted source audit with regression coverage, not a claim that every feature or platform has been exhaustively tested.

## Findings and fixes

| Area | Confirmed issue | Fix |
| --- | --- | --- |
| Radio settings | Search limit, broken-station filtering and defaults were read only during initialization. | Reload them through the existing main-window settings reload path. Restart the search when relevant saved settings change. |
| Radio requests | Concurrent searches could append obsolete results to a newer search. Repeated requests could start parallel server discovery. | Cancel superseded search replies, finish their progress tasks and share one discovery attempt. Add transfer timeouts. |
| Pagination | Repeated Load more clicks could skip pages; a failure advanced the offset permanently. | Disable concurrent pagination and advance the offset only after successful completion. |
| Country filter | Populating the combo box emitted intermediate change signals and started unwanted searches. | Block signals while loading countries/defaults and use the saved country before discovery completes. |
| Radio activation | Search-result double-click did not set the normal playlist activation flag. | Forward the same activation intent used by the other media views. |
| File-tree roots | Prefix matching could remove a parent root when a nested root was selected. | Walk to the selected model entry's top-level root. No filesystem deletion is performed. |
| File activation | Enter could use a list index while tree mode was active. | Route activation through the originating model index and handle it once. |
| Path entry | Every text change could navigate to a partially typed existing directory and add history entries. | Navigate when Enter is pressed. |
| Appearance | Cancel after Apply could restore the pre-Apply theme; a page opened after custom colors were applied could mistake them for system colors. | Save the applied palette/style baseline and use Appearance's saved system palette. |
| Background controls | Stretch could enable crop while aspect-ratio preservation was off. | Derive dependent control states together during loading and editing. |
| Decimal settings | Reinitialization retained old double-spinbox snapshots, causing repeated saves on later Apply operations. | Clear decimal snapshots along with the other widget snapshots. |
| Documentation/CI | README directed Orange users to upstream builds/support; inherited CI excluded Orange pushes. | Document Orange's own behavior and add a dedicated Linux build/test workflow. |

## Verification

The new `gui_regressions_test` covers repeated Apply, appearance rollback and system palette restoration, background dependencies, nested-root removal, keyboard activation, obsolete radio replies, shared discovery, pagination retry and live settings reload. Radio responses are simulated deterministically.

Existing tests cover playlists, collection/database operations, parsers, utilities, waveform processing and desktop/release metadata. See the Orange CI result for the exact tested commit. Live provider tests are separate and do not run in the offline suite.

## Remaining validation limits

- Native KDE/Wayland visual inspection and physical audio outputs, MTP/iPod devices and audio CDs require a suitable desktop/device environment.
- Authenticated streaming, scrobbling and live cover/lyrics services require working accounts and external endpoints. Their main GUI entries being connected is not proof of provider availability.
- macOS and Windows packaging/signing and release binary publication are outside this patch's verification.
- Mounted network-share enumeration is connected to the Files menu; mounting new shares is delegated to the system file manager.
- Waveform mode is already connected to playback and settings; existing waveform tests remain part of the build.
