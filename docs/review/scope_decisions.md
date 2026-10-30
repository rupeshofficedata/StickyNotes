# Scope Decisions for Release 1

* **Source:** Stage 8 reconciled view (`intermediate_zoom_out_report.md`). Decision 1 was chosen by the owner. Decisions 2 to 6 use the Stage 8 recommendation, as the owner asked ("use the recommended option for all automatically").
* **Status:** Recorded here only. `docs/designs/sticky_notes.md` has not been edited.
* **Reverses earlier owner choices:** decisions 2, 3, 4 and 6 reverse something the owner chose or asked for earlier. They are marked below so the owner can veto any of them.

| # | Decision | Chosen | What it removes from the design | Reverses an earlier owner choice? |
|---|---|---|---|---|
| 1 | Folder change | **Use this folder only.** Changing the folder points the app at it. Settings has an "Open notes folder" button. The user moves files by hand. If the chosen folder is empty, the app says so. | `FolderMover`, `tst_foldermover`, the Move progress bar, CUJ 5 copy and delete steps, the Move goal | Yes. Earlier choice was Move. Owner chose this option just now. |
| 2 | External changes | **Rescan on focus plus a Refresh button.** The "Changed outside this app" banner compares modification times and shows for an open note with unsaved edits. Reload discards unsaved edits. Keep mine overwrites on the next save. No file watcher. | `FolderWatcher`, `tst_folderwatcher`, spike S4, the "Watch for external changes" setting, per-note-folder watches | Yes. Earlier choice was watcher plus rescan. A note left open on the desktop stays stale until the user clicks it. |
| 3 | Window geometry | **`QSettings` stores size, and position on X11 only.** Cascade is a few lines in the note-window opener. | `GeometryStore`, `WindowPlacer`, `geometry.json`, session-management restore, spike S2 | Yes. Earlier choice was a state file outside the notes folder. `QSettings` is also per machine and outside the notes folder, so that goal still holds. |
| 4 | Hide behavior | **Derived from tray presence, no setting.** Closing the primary window hides it. With a tray, the app stays running. With no tray, closing the last note window quits. Three-row lifecycle table. | The hide-mode setting, the six-row matrix, live tray watching | Yes. The owner asked for a choice between hiding in the tray and hiding in the background. This removes the user-facing choice. |
| 5 | Spikes | **Three:** S1 (note format), S3 (Flatpak and Qt floor), S6 (image paste, only if suspect). Results go in the design with a one-line fallback each. | S2, S4, S5, the separate `docs/spike-results.md`, six spike directories | No |
| 6 | Release channels | **AUR first.** Flathub follows once S3 passes. The reverse-DNS app ID stays. | The Flatpak manifest from release 1, portal folder-access constraints on the folder goal | Yes. The owner chose AUR plus Flathub. |

## Further cuts from the same reconciled view

These follow the Stage 8 recommendation and also apply:

* No `meta.json` schema version and no unknown-field retention. Keep folder per note.
* Cut extra colors beyond a small fixed set, the color-scheme override, image downscale and the 10,000-note search ceiling.
* Show unreadable notes as an error row in the list and leave the folder in place. No `.bad` quarantine.
* Call `KDBusService` and `KStatusNotifierItem` directly. Delete the `SingleInstance`, `TrayIcon` and `AppSettings` wrapper classes. Keep one `PlatformCaps`.
* Reduce the Detailed Implementation Plan to one-line responsibilities and keep only cross-boundary contracts.
* Fold Alternatives D to I into one-line rationales in the decisions table.

## Gaps to add (hold whichever way the scope goes)

* Provisional note format `.html`, with what changes if S1 picks Markdown.
* Note ID form: lowercase UUID v4, no braces.
* Image contract: PNG, naming rule, one size cap, relative paths confined to the note's `images/` folder, no remote loads, orphans left in place.
* C++/QML interface: objects exposed to QML, and who creates each note window's `RichTextBridge`.
* Save-failure contract and the quit prompt (save, discard, cancel). Honest loss window from the debounce. Use Qt's quit path and `aboutToQuit`.
* Fix the core library layering for `RichTextBridge`.
* A short Risks section: plaintext notes in a synced folder, the loss window, the KDE Frameworks footprint, Wayland position limits, stale open notes.
* One Qt floor value (6.5 provisional).
* Fix "survive uninstalls" wording, add or drop `DBusActivatable=true`, delete the "retention rule" phrase.
