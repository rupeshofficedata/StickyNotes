# Zoom Out Report (Stage 5)

* **Target:** `docs/designs/sticky_notes.md` (full file, lines 1 to 519)
* **Standard:** `zoom-out-standard.md`
* **Reviewer:** egm-zoom-out. The project is greenfield, so no codebase reuse search was possible. Reuse findings cover platform and library reuse only.

**Verdict:** Context is sufficient. The document states what and why clearly, and root-problem focus is good. The design is over-scoped for a first release, and the implementation plan drops into code-level detail in several places.

**Owner decisions in conflict:** findings 1, 2, 4, 8 and 11 recommend reversing choices the owner made during drafting (folder Move, watcher plus rescan, geometry state file, two hide modes as a setting, AUR plus Flathub). Those choices stand until the owner decides otherwise at the checkpoint.

## Blocker

1. **Folder Move is a speculative, high-risk feature in release 1** (lines 27, 87-91, 179-188, 273-277, 370-374; FolderMover). It builds a copy, verify and delete engine with progress UI, name-clash refusal, watcher pause and open-note closing. It protects against data loss but is itself the largest data-loss risk in the app. The need "pick where notes live" is met without it.
   *Fix:* for v1, the folder setting means "use this folder". Never move or delete anything. Tell the user to move files in the file manager, or offer "Open notes folder". If Move stays, drop only the delete step and leave the old folder in place. That removes `FolderMover.*`, `tst_foldermover`, the progress bar and the delete branch of CUJ 5.

## Major

2. **External-change handling is heavy** (lines 28, 93-97, 190-195, 279-283, 365-368, 513). Watcher, rescan on focus, Refresh button, degraded mode, own-write echo list, per-note-folder watches that grow with note count, a settings toggle and a three-way conflict banner.
   *Fix:* v1 uses rescan on focus plus Refresh. Keep the "Changed outside this app" banner, triggered by comparing modification times in the rescan. Add `QFileSystemWatcher` later behind the same `rescan()` interface. Drops the setting, `FolderWatcher.*`, its test and spike S4.
3. **Spike stage is too large** (lines 56, 225-232, 324-338). Six spikes, a results file and six directories before any app code. S2 (session management, a stated bonus) and S5 (tray on four desktops) gate no decision.
   *Fix:* keep S1 (file format), S3 (Flatpak and Qt floor) and S6 only if paste is suspect. Fold S2 and S5 into later per-desktop checks. Record results in a section of the design, not separate files.
4. **Window position and session management is a speculative bonus carried as full components** (lines 29, 48-49, 197-201, 375-378, 415-418; GeometryStore, WindowPlacer, Stage 4). On Wayland the app cannot place windows.
   *Fix:* v1 stores size, and position on X11, in `QSettings`. Delete `GeometryStore`, its cleanup job and `WindowPlacer`. Keep cascade as a few lines in the note-window opener.
5. **Two persistence mechanisms for app state** (lines 29, 117, 375-383). `QSettings` plus a hand-rolled JSON state file. `AppSettings` is a pass-through wrapper.
   *Fix:* use `QSettings` for everything machine-local, with no wrapper class.
6. **The platform layer is one-line wrappers** (lines 136-140, 400-418). `SingleInstance` wraps `KDBusService`; `TrayIcon` wraps `KStatusNotifierItem`.
   *Fix:* use both directly in `main.cpp` and `AppLifecycle`. Keep one small `PlatformCaps`. Delete `SingleInstance.*` and `WindowPlacer.*`.
7. **Altitude drops in the Detailed Implementation Plan** (lines 343-433). It lists `find_package` components, construction order, signal handlers, method signatures, millisecond values (300 ms and 400 ms for the same idea) and per-file test assertions.
   *Fix:* reduce component entries to one-line responsibilities. Keep only cross-boundary interfaces: `NoteStore` signals and the lifecycle table.
8. **AppLifecycle table is over-specified** (lines 36, 158-171, 385-388, 464). Two hide modes times tray present or absent gives a six-row matrix.
   *Fix:* one rule set. Close the primary window: hide it. Close the last note with the primary hidden: quit unless a tray exists. Drop the hide-mode setting and derive it from tray presence.
9. **Features beyond the stated problem** (lines 31-35, 51, 214, 423): six note colors, a color scheme override setting, an image size cap with downscale, a 10,000-note search ceiling with lazy parsing, and `.bad` quarantine of unreadable notes.
   *Fix:* cut each. Show unreadable notes as an error entry in the list and leave the folder untouched.
10. **Folder-per-note with a separate `meta.json` and unknown-field retention** (lines 116, 152, 353, Alternative D). Keep the folder unit, since it is the right call for images. Put color and timestamps in the note file header or use file modification time. Drop unknown-field retention and the schema version until a second writer exists.
11. **Distribution scope doubles the work** (lines 13, 40, 234-236, 482-491). A Flatpak manifest, portal folder-access limits and a sandbox tray permission.
    *Fix:* ship the AUR first and make Flathub a follow-up gated on S3. Keep the reverse-DNS app ID now.

## Minor and nits

12. Qt floor is stated as 6.5 in one place and "decided by S3" in another. Pick one.
13. Goals mix requirements, design decisions and implementation notes (lines 23-41). Move mechanism detail to the Technical Plan.
14. `main.cpp` installs raw SIGTERM and SIGINT handlers (line 348). Qt's `aboutToQuit` flush covers normal exits. Use a `QSocketNotifier` helper only if tests show missed flushes.
15. Debounce is described as about 400 ms for saves and about 300 ms for watch events. Use one shared constant or document none.
16. An 80% line-coverage target on `src/core` is arbitrary. Replace with tests for data-loss paths: save failure, bin, lifecycle.
17. Four directory READMEs are a deliverable (lines 502-505). One `src/README.md` after the code exists is enough.
18. The Alternatives section is long. Keep A to C and fold D to I into one-line rationales in the decisions table.

## Keep

`QSaveFile` for atomic writes. `KDBusService` and `KStatusNotifierItem` instead of hand-rolled D-Bus. XDG base directories. A C++ core with no QML dependency. Refusing always-on-top. No database or index. Token-based colors. A clear Non-Goals list.

## Reviewer's minimal v1 shape

QML windows, `NoteStore`, `NoteIndex`, `RichTextBridge`, `AppLifecycle` with the simple rule, `QSettings` for folder and window size, `KDBusService`, `KStatusNotifierItem`, rescan on focus with a conflict banner, in-app bin, AUR package. Deferred: folder Move, file watcher, geometry file and `WindowPlacer`, Flathub, extra spikes, color-scheme override, image downscale.
