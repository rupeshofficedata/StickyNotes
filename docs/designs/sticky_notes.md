# StickyNotes: Sticky Notes for Linux

*   **Author:** rupeshofficedata
*   **Status:** Draft (revised after review stages 5 to 9)
*   **Date:** 2026-10-28

--------------------------------------------------------------------------------

## Overview

StickyNotes is a desktop app for KDE Plasma on Wayland. It keeps short notes as separate small windows on the desktop, in the style of the Windows Sticky Notes app. A primary window lists, searches and manages all notes. Each note opens in its own window. Notes are saved as files in a folder the user can change, so they persist across restarts, crashes and upgrades. The app never deletes the notes folder.

Plasma 6.4 and newer is the tested floor. The app also runs on other desktops (GNOME, Sway, X11) with fewer conveniences, and the README documents the differences. The first release ships on the AUR. Flathub follows later.

## The Problem

Linux has no sticky-notes app that matches what Windows Sticky Notes offers: free-floating note windows with colors, text formatting and pasted images, plus a list to find them again. The options that exist are tied to other tools, store notes in opaque databases, or do not handle images.

On Wayland there is a second problem. Applications cannot place or restore their own windows, so a naive port would open every note in a random place after each login. The app has to work with what the compositor allows and say clearly which features are missing.

The proposed solution is a Qt 6 and QML app with one primary window and one window per note. Notes are plain files in a default folder that the user can change.

## Goals

*   **Persistent notes.** Notes are files and survive restarts, crashes and upgrades. The app never deletes the notes folder. A failed save is shown to the user and is never silent.
*   **Changeable save location.** Notes go to `$XDG_DATA_HOME/io.github.rupeshofficedata.StickyNotes/notes` by default. The settings panel lets the user pick another folder. The app remembers the choice and does not copy or move any notes. If the chosen folder is missing, read-only or unmounted, the app says so and does not fall back to an empty store.
*   **Note windows.** Each note is its own window. New notes cascade from the last one. The default color is yellow in the light theme and near-black with light text in the dark theme.
*   **Formatting.** Bold, italic, underline, strikethrough, bullet lists and a small fixed set of note colors. Formatting survives save and load unchanged.
*   **Images.** Images can be pasted or dragged into a note and are saved with it.
*   **Primary window.** Lists notes, searches them, and holds settings and the Bin.
*   **Delete.** Deleting a note moves it to an in-app bin inside the notes folder. The user can restore a note or empty the bin.
*   **Hide and reopen.** The primary window hides when closed. Every note window has a list button that reopens it.
*   **Defined exit rules.** With a system tray, the app keeps running until the user chooses Quit. Without a tray, the app quits when no window is left.
*   **Single instance.** Launching the app again shows the primary window. A `.desktop` launcher puts it in the application menu.
*   **Notes changed by outside tools.** The user may keep the notes folder inside a folder that Nextcloud or Google Drive already syncs, for backup. The app rescans the folder when the application becomes active and on a Refresh button. It never overwrites a newer outside edit without the user choosing.
*   **Dark mode.** The app follows the system color scheme.
*   **Plasma first.** All of the above works on Plasma 6.4+ on Wayland, the tested floor. On other desktops notes remain usable and the README lists the differences.

## Non-Goals

*   Built-in cloud sync or backup. The app does not run rclone or talk to cloud services. A folder that an outside client already syncs is supported.
*   Moving or copying notes when the folder changes. The user moves files in a file manager.
*   A file watcher for live updates. Changes appear when the application becomes active or on Refresh.
*   A choice between hiding in the tray and hiding in the background. The behavior follows tray presence.
*   Window position restore on Wayland, a geometry state file, and `xdg-session-management-v1`.
*   Always-on-top windows. There is no confirmed Wayland standard. Users can set a compositor window rule.
*   A temporary (`/tmp`) storage option, and a first-run folder prompt.
*   A database or search index.
*   Encryption, user accounts, tags, plugins, custom themes, a free color picker, and telemetry.
*   A Flatpak in the first release.

## Open Questions

*   **Note file format.** `note.html` written by `QTextDocument::toHtml`. Spike S1 passed for HTML on Qt 6.12 (see Spikes). The test used `QTextDocument`, not a QML `TextEdit`, so Stage 1 repeats the round trip through a real `TextEdit`. `toHtml` output may differ between Qt versions, which would rewrite a note once after a Qt upgrade.
*   **Qt floor.** Set by the KDE Frameworks version, not by the app code. KF6 6.31 (current Arch) requires Qt 6.10.0 (spike S3). The app code itself needs only the `Qt.styleHints.colorScheme` API (Qt 6.5), which is unverified. Building against an older KF6 would allow an older Qt.
*   **Spike results.** The Spikes section holds the pass rules. Results are added there.

## Critical User Journeys (CUJs)

### CUJ 1: First launch and first note

*   **Given**: The app is installed, no notes folder exists, the desktop is Plasma 6.4+ on Wayland.
*   **When**: The user starts the app from the application menu.
*   **Then**: The app creates the default folder (it creates a missing folder only when the path is the default), shows the primary window with an empty list and a "New note" button. Clicking it opens a yellow (light theme) or near-black (dark theme) note window. No folder prompt appears.

### CUJ 2: Write a note with an image and find it later

*   **Given**: A note window is open.
*   **When**: The user types text, applies bold and a bullet list, pastes a screenshot, and closes the note. Later the user types a word from the note into the search box.
*   **Then**: The note is saved in its own folder with its text and image. The search lists the note. Opening it shows the same formatting and image.

### CUJ 3: Close the primary window and get back

*   **Given**: Two notes are open and the primary window is visible.
*   **When**: The user closes the primary window, then clicks the list button on a note.
*   **Then**: The primary window hides, and the notes stay open. The list button shows the primary window again.

### CUJ 4: Last note closes with no tray

*   **Given**: No system tray exists, the primary window is hidden and one note is open.
*   **When**: The user closes that note.
*   **Then**: Pending saves finish and the app quits. Launching it again shows the primary window with all notes intact.

### CUJ 5: Point the app at another folder

*   **Given**: Notes exist in the default folder.
*   **When**: The user opens settings, picks another folder, and confirms.
*   **Then**: The app saves pending edits, closes all note windows, switches to the new folder and does not touch the old one. If the new folder has no notes, the list is empty and the app says "No notes in this folder". The settings page has an "Open notes folder" button so the user can move files in the file manager, then press Refresh. A chosen folder that does not exist is an error.

### CUJ 6: Note changed by a sync client

*   **Given**: The notes folder lies inside a Nextcloud folder, and a note is open.
*   **When**: The sync client writes a newer version of that note and the user focuses the note window.
*   **Then**: A note with no unsaved edits reloads silently. A note with unsaved edits shows a banner "Changed outside this app" with Reload (discards the edits) and Keep mine (the next save overwrites the file).

### CUJ 7: Delete and restore

*   **Given**: A note exists.
*   **When**: The user deletes it, opens the Bin view, and restores it.
*   **Then**: The note folder moves to `.bin` and leaves the main list. Restore moves it back with its images.

## Technical Plan (Architecture)

### Summary

A Qt 6 and QML app with a C++ core. QML draws the primary window and the note windows and sends user intent to C++. C++ owns notes on disk, the note list, the app lifecycle and rich-text editing. KDE Frameworks components are used directly with no wrapper classes: `KDBusService` for the single instance and `KStatusNotifierItem` for the tray.

### Decided design choices

| Area | Decision |
|---|---|
| UI | Qt Quick (QML) windows with a C++ rich-text helper |
| Storage unit | One folder per note, named by note ID: `note.html`, `meta.json`, `images/` |
| Folder change | Point the app at the new folder. No copy or move. "Open notes folder" button |
| External changes | Rescan when the application becomes active, and on a Refresh button. No watcher |
| Window state | C++ `QSettings` owned by `NoteStore`, keyed by note ID: size, and position on X11 only. No state file |
| Delete | Move the note folder to `.bin`. Kept until the user empties the bin |
| Hide behavior | Derived from tray presence. No setting |
| Single instance | `KDBusService`, `DBusActivatable=true` with a D-Bus service file |
| Tray | `KStatusNotifierItem`, created only if the StatusNotifierWatcher exists at startup |
| Release channel | AUR first. Flathub later |
| Quit | `quitOnLastWindowClosed` is off. `AppLifecycle` decides |

Further choices, each with the reason the alternative lost, are in Alternatives Considered.

### Components

```text
QML (UI only, sends intents)
  Main.qml: PrimaryWindow  NoteWindowManager -> NoteWindow
  SettingsPage  BinView  ErrorBanner  QuitPrompt  Theme
        |
        v
App target (links Qt Quick, Qt Widgets, KF6)
  main.cpp: KDBusService, KStatusNotifierItem, UnixSignalBridge
  RichTextBridge(QQuickTextDocument + QTextCursor, paste, drop)
        |
        v
Core library (Qt Core, Gui, DBus; no Qt Quick)
  NoteStore  NoteIndex(list model + filter)  AppLifecycle  PlatformCaps  Logging
        |
        v
Disk
  <notes>/<id>/note.html  <notes>/<id>/meta.json  <notes>/<id>/images/
  <notes>/.bin/<id>/...
  QSettings: notes folder path, per-note size and X11 position
```

### Note on disk

*   `<id>` is a UUID v4 in lowercase with dashes and no braces, from `QUuid::toString(QUuid::WithoutBraces)`.
*   `note.html` holds the text and formatting, written by `QTextDocument::toHtml`. Image links are relative: `images/<uuid>.png`. Provisional until spike S1.
*   `meta.json` holds the color token only, for example `{"color": "default"}`. If it is missing, unreadable or holds an unknown token, the note loads with the `default` token. The modified time is the modification time of `note.html`. If a second field is ever added, add a version field then.
*   `images/` holds the note's images as PNG files named `<uuid>.png`. The 10 MB limit applies to the source bytes. Larger images are rejected with a message. Non-PNG images are re-encoded to PNG, and animated GIFs lose their animation. Non-image drops are ignored.
*   A missing image file shows a placeholder. Images that the text no longer references stay in `images/` until the note is deleted forever.
*   A note folder whose files cannot be read appears in the list as an error row. Clicking it shows the reason. The app leaves the folder untouched.
*   Moving a note to the bin: if `.bin/<id>` already exists, the older entry is renamed to `.bin/<id>.<unix-ms>` first. Restore is refused if `<notes>/<id>` already exists. The note stays in the bin and the app says "A note with this ID already exists."
*   Delete forever removes `.bin/<id>`, then the note's `QSettings` entries.

### Note colors

A note stores one of four tokens. Each token has a fixed background and text color for light and dark. These are v1 values and can be changed in `Theme.qml`.

| Token | Light background | Dark background | Light text | Dark text |
|---|---|---|---|---|
| `default` | `#FFF2A8` | `#1C1C1C` | `#1E1E1E` | `#EDEDED` |
| `blue` | `#CFE8FF` | `#1F2A36` | `#1E1E1E` | `#EDEDED` |
| `green` | `#D4F3D0` | `#1F2D20` | `#1E1E1E` | `#EDEDED` |
| `pink` | `#FFD6E5` | `#36222B` | `#1E1E1E` | `#EDEDED` |

The color applies to the whole note, not to selected text. The note toolbar shows the four colors as a fixed palette.

### Lifecycle (AppLifecycle)

`AppLifecycle` is the only code that hides, shows or quits. A window counts as visible when it is shown on screen. A minimized window counts as visible. Tray presence is checked once at startup. If the tray disappears later, the behavior does not change.

| Event | Tray present | No tray |
|---|---|---|
| Close primary window | Hide it. App keeps running | Hide it if a note window is visible. Otherwise quit |
| Close a note window | App keeps running | If it was the last visible window, quit. If the primary window is visible, keep running |
| List button, relaunch, tray click | Show and focus the primary window | Show and focus the primary window |
| Quit (primary or tray menu) | Flush saves, quit | Flush saves, quit |

At startup the primary window is shown. With no tray, the app shows once: "No system tray found. The app quits when you close the last window."

### Saving

*   Edits are written 400 ms after the last keystroke. `save` writes `meta.json` first and `note.html` second, each through `QSaveFile` (write to a temp file, then rename). A crash between the two leaves the old text with the new color, which is harmless.
*   The app flushes on note close, on quit, and in the `aboutToQuit` handler. SIGTERM and SIGINT call `AppLifecycle::quit(force=true)` through a `UnixSignalBridge` (a socket pair read by a `QSocketNotifier`) in `main.cpp`. A session logout (`commitDataRequest`) does the same.
*   The app does not call fsync on the directory. A power loss can lose the latest save. A crash can lose up to 400 ms of typing.
*   If a save fails, the note window shows a banner with the reason and keeps the text in memory. The app retries on the next edit and every 10 seconds while the note is open.
*   Quit sequence: `AppLifecycle::quit()` calls `NoteStore::flushAll()`. If it returns note IDs, the lifecycle emits `quitBlocked(ids)` and returns. `QuitPrompt.qml` shows Retry (calls `quit()` again), Discard and quit (calls `quit(force=true)`) and Cancel (does nothing). `aboutToQuit` runs one last `flushAll()` and never opens a dialog. The tray item's Quit action is overridden to call `AppLifecycle::quit()`. `quit(force=true)` skips the prompt, so a SIGTERM during a failed save loses the unsaved text.
*   Saving a note whose folder was removed outside recreates the folder, `meta.json` and `note.html`.

### Folder change

*   The folder setting stores the new path. The app does not copy, move or delete notes.
*   The app creates a missing folder only when its path is the default (`QStandardPaths::GenericDataLocation` + `/io.github.rupeshofficedata.StickyNotes/notes`). A user-chosen folder that is missing, read-only or unmounted is reported in the primary window. The app never creates an empty store in its place.
*   If the new folder is empty, the app says "No notes in this folder".

### Notes changed by outside tools

`NoteStore` keeps a registry of open notes: `{id, recordedMtimeNs, recordedSizeBytes (for both note.html and meta.json), dirty}`. It records the pairs after every load and every successful save. `NoteStore::rescan()` runs on the GUI thread when the application becomes active (`applicationStateChanged` to `Qt::ApplicationActive`) and on Refresh. There is no debounce, because the trigger is a user action.

*   For each open note it reads the pairs for both files. A missing `note.html` raises `noteRemovedOutside(id)`. Any difference from the recorded pairs raises `noteChangedOutside(id)`.
*   For closed notes it compares with the index cache and updates rows. New folders appear and missing folders disappear.
*   A note that fails to parse stays an error row and is tried again on the next rescan. This covers a note a sync client is still writing.
*   An outside color change on a note with no unsaved edits updates the color.

| Open note state | File changed outside | File removed outside |
|---|---|---|
| No unsaved edits | Reloads silently | Closes with a message "Note removed outside this app" |
| Unsaved edits | Banner: Reload or Keep mine | Banner: "Removed outside this app". Save recreates it |

Reload replaces the text and discards unsaved edits. Keep mine updates the recorded pairs to the current files, so the banner returns only if the files change again, and the next save overwrites them.

### Window placement and state

*   New notes open at (80, 80), then each offset 32 px from the last, and wrap to (80, 80) after 8 notes opened in the current session. `AppLifecycle::nextCascadePosition()` computes it. Wayland ignores the position.
*   Default note size is 280 x 320. Minimum is 200 x 160.
*   `NoteStore` stores size by note ID in `QSettings`. It stores position only on X11. The entry is removed when a note is deleted forever.

### Rich text and images

*   `RichTextBridge` works on the `QQuickTextDocument` of a QML `TextEdit` whose text format is rich text. It exposes bold, italic, underline, strikethrough and bullet list as commands to QML.
*   The bridge sets the document base URL to the note folder and filters `img src` values on load, so only `images/<uuid>.png` inside the note folder loads. Remote URLs never load.
*   Text paste is plain text, so foreground colors from other apps do not make text unreadable on a dark note.
*   A paste handler reads image data from `QClipboard`, saves a PNG into the note's `images/` folder, and inserts it. Dropped image files take the same path through a QML `DropArea`. Images display scaled down to the window width.

### C++ and QML interface

*   `NoteStore`, `NoteIndex` and `AppLifecycle` are single instances registered as QML singletons with `qmlRegisterSingletonInstance` in `main.cpp`.
*   `Main.qml` is the root (not a window). It holds `PrimaryWindow` and `NoteWindowManager`, so the manager lives while the primary window is hidden. `AppLifecycle::openNote(id)` emits `openNoteRequested(id)`, and the manager creates one `NoteWindow` with `Component.createObject`.
*   Each `NoteWindow` creates its own `RichTextBridge` (registered with `QML_ELEMENT`) and binds it to its `TextEdit`. Each window reports `windowShown(kind, id)` and `windowHidden(kind, id)` to `AppLifecycle` from `onVisibleChanged`.
*   Note windows always draw their own header (drag area, list button, delete button), so no server-side-decoration detection is needed.
*   Delete is a button in the note window header and a "Delete" entry in the context menu of each list row. Deleting an open note saves it, moves it to the bin, and closes its window.
*   Class interfaces are in the Detailed Implementation Plan.

### Search and list

*   `NoteIndex` reads the title and plain text of every note at startup and again whenever a note's modification time changes. It is tested with 1,000 notes. No larger ceiling is promised.
*   Search is a case-insensitive substring match over the title and plain text. The Bin is not searched.
*   The title is the first non-empty line, cut to 60 characters. An empty note is titled "Untitled".
*   The list is sorted by the modification time of `note.html`, newest first.

### Platform differences

*   `PlatformCaps` reports two facts: tray present, and Wayland or X11. Code asks `PlatformCaps` and never checks the desktop by name.
*   Tray present means `org.kde.StatusNotifierWatcher` is registered on the session bus.
*   Note windows are frameless and use `startSystemMove()` and `startSystemResize()` from their own header.

### Logging

*   `QLoggingCategory` entries `stickynotes.storage`, `stickynotes.tray` and `stickynotes.activation`. Users can turn them on with `QT_LOGGING_RULES`. No telemetry.

### Spikes

The spikes are throwaway programs in a `spikes/` directory that is not part of the build. Results go in the Result column.

| Spike | Question | Pass | If it fails | Result |
|---|---|---|---|---|
| S1 | Does rich text with underline, strikethrough, bullets and an image round-trip through `note.html`? | A second save is byte-identical and the formatting survives | Save only when the document text changes. If the drift remains, switch to Markdown and drop underline | **Pass for HTML** (Qt 6.12, `QTextDocument` only): save 1, 2 and 3 are byte-identical and bold, italic, underline, strikethrough, bullet list and image survive. Markdown fails: underline is written as `_text_` and read back as italic. Not tested through a QML `TextEdit` yet |
| S3 | Does a skeleton app build and start against Qt and the KF6 packages `KF6DBusAddons` and `KF6StatusNotifierItem`? Does `KStatusNotifierItem` need Qt Widgets? | Builds and starts. The Widgets answer and the Qt floor are recorded | Raise the Qt floor to the lowest version that builds | **Pass on Qt 6.12 and KF6 6.31.** Findings: (1) `find_package(KF6 COMPONENTS ...)` does not work; each framework is its own package. (2) `KStatusNotifierItem` needs `QApplication` and Qt Widgets; with `QGuiApplication` the app aborts with "Cannot create a QWidget without QApplication". (3) KF6 6.31 requires Qt 6.10.0. (4) `kdbusaddons` and `kstatusnotifieritem` pull in `kwindowsystem`. (5) `KDBusService` registers `io.github.rupeshofficedata.StickyNotes` with organization domain `rupeshofficedata.github.io` and application name `StickyNotes`. (6) A second launch exits with status 0 and the first receives `activateRequested`. (7) With no StatusNotifierWatcher, `KStatusNotifierItem` logs "SNI unavailable" and the app keeps running. Not tested: a Qt 6.5 build, Wayland activation tokens, a real tray host |
| S6 | Does paste and drop work from Spectacle, Firefox and Dolphin? | An image appears in the note | Fix the C++ clipboard handler. S6 is not a gate and runs while building Stage 1 | not run |

The development machine has Qt 6.12 and KF6 6.31. The spike programs are in `spikes/s1_richtext/` and `spikes/s3_skeleton/`. Qt logs go to journald in this environment, so set `QT_FORCE_STDERR_LOGGING=1` to see them in a terminal.

### Risks and accepted trade-offs

| Risk | Handling |
|---|---|
| Notes are plain files. Anyone with access to the folder, or the cloud provider, can read them | Accepted for v1. Encryption is a Future Improvement |
| Up to 400 ms of typing can be lost in a crash, and a power loss can lose the last save | Accepted. Stated in the README |
| A note left open does not update until the application becomes active or the user presses Refresh | Accepted |
| Notes reopen in compositor-chosen places on Wayland | Accepted. The README says so |
| Tray detection happens once. If the tray goes away later, the app does not adapt | Accepted |
| A sync client may write a half-finished note | Shown as an error row and loaded on the next rescan |
| KDE Frameworks adds dependencies for non-KDE users, pulls in Qt Widgets and `kwindowsystem`, and sets the Qt floor (KF6 6.31 needs Qt 6.10) | Accepted |
| Pasted images can be large | Rejected above 10 MB |
| Image links in HTML could point outside the note | The loader accepts only `images/<uuid>.png` in the note folder |

## Alternatives Considered

### Alternative A: Plasma widget (plasmoid) instead of a standalone app

*   **Pros**: Sits on the desktop layer, so Wayland's rule against self-positioning does not apply. Native Plasma look.
*   **Cons**: Plasma only. Does not fit one primary window plus free-floating note windows. Harder to publish as a normal app.
*   **Reason for Rejection**: The design needs separate note windows, a tray and a lifecycle, and it supports other desktops.

### Alternative B: Electron or Tauri

*   **Pros**: Works on Wayland and other platforms. Web editors give rich text and images cheaply.
*   **Cons**: Heavier memory and slower start. Weaker KDE integration for tray and theme.
*   **Reason for Rejection**: Qt is native on Plasma and starts fast.

### Alternative C: Qt Widgets with `QTextEdit`, or QML plus Widgets

*   **Pros**: Widgets give the rich-text editor for free. Mixed mode uses the best tool per window.
*   **Cons**: Widgets need more stylesheet work for note colors and frameless windows. Mixed mode links two UI stacks and doubles boundary bugs.
*   **Reason for Rejection**: The owner chose QML with a C++ rich-text helper and accepted the glue code for formatting and paste.

### Other decisions

| Decision | Rejected option | Reason |
|---|---|---|
| Image storage | Inline base64; shared assets folder | Base64 rewrites large files on every save. A shared folder leaves orphans and breaks single-file copies. Folder per note keeps a note and its images together |
| Geometry | Sidecar file; state file in the notes folder; state file outside it | A sync client carries window positions between machines. `QSettings` is per machine, standard and enough |
| Settings API | The QtCore `Settings` QML type | It uses declared properties and does not fit per-note keys. `NoteStore` needs the folder path in C++ before QML loads |
| Delete | XDG trash; permanent delete | Trash is unreliable on other drives and sandboxes. Permanent delete is the one path that loses notes |
| Folder change | Move; copy and keep the old folder | Move is the largest data-loss risk and needs hashes, a progress marker and recovery rules. A file manager does the job with no app code |
| External changes | File watcher; watcher plus rescan | A watcher needs one watch per note folder, debounce, own-write filtering and a setting. Rescan covers sync clients for notes that are not left idle |
| Hide behavior | A setting for tray or background | The user-visible difference is small. Tray presence decides, which shortens the lifecycle table |
| Sync and encryption | Built-in rclone sync; encrypted notes | Both need a conflict policy and key handling. Deferred to Future Improvements |
| Storage extras | `/tmp` option; first-run prompt; always-on-top | `/tmp` loses notes on reboot. A default folder plus a settings entry replaces the prompt. No Wayland standard exists for always-on-top |
| Release scope | AUR plus Flathub in release 1 | A Flatpak adds portal folder-access limits and a tray permission. AUR first, Flathub later |

--------------------------------------------------------------------------------

### Future Improvements

*   **Live updates:** a file watcher with debounce and own-write filtering, behind the existing `rescan()` function.
*   **Folder Move:** a tested copy, verify and delete flow, with a progress marker, if users ask for it.
*   **Window position restore on Wayland:** `xdg-session-management-v1` and a geometry store, if Qt and KWin support it.
*   **Hide mode setting:** a choice between tray and background.
*   **Flathub:** a Flatpak manifest, with checks of the KDE runtime Qt version, portal folder access across restarts, and the tray permission.
*   **Built-in optional sync or backup:** rclone-based, with a conflict policy and a way to run rclone from the Flatpak sandbox.
*   **Encryption at rest.**
*   **Diagnostics report:** a "Copy diagnostics" button listing Qt version, platform plugin, desktop and tray state.
*   **Color-scheme override and image downscale.**

--------------------------------------------------------------------------------

## Detailed Implementation Plan

*   **Name:** `StickyNotes`. **Executable:** `stickynotes`.
*   **App ID:** `io.github.rupeshofficedata.StickyNotes`.
*   **License:** GPL-3.0-or-later.
*   **Language and build:** C++17, QML, CMake 3.21 or newer.
*   **Qt minimum:** 6.10, set by KF6 6.31 (see Spikes). The app code itself should need only 6.5.
*   **KDE Frameworks 6 components:** `DBusAddons` and `StatusNotifierItem`.

All files are new. Paths are relative to the project root.

### Stages

| Stage | Files | Exit check |
|---|---|---|
| 0 | Spikes S1 and S3 in `spikes/` (not part of the build) | The Spikes table has a result for S1 and S3, or names the fallback taken. If S1 selects Markdown, change before Stage 1: the note file name and extension, the `NoteStore` serializer, `tst_notestore.cpp`, `tst_richtextbridge.cpp`, and the spec lines that mention underline |
| 1 | `CMakeLists.txt`, `.clang-tidy`, `main.cpp` (no tray and no D-Bus yet), `NoteStore`, `NoteIndex`, `Logging.h`, `RichTextBridge` (formatting, paste, drop), `Main.qml`, `Theme.qml`, `PrimaryWindow.qml`, `NoteWindow.qml`, `NoteWindowManager.qml`, `ErrorBanner.qml`, `tests/CMakeLists.txt`, `tst_notestore`, `tst_noteindex`, `tst_richtextbridge` | The build passes, those tests pass, and a note can be created, edited, closed, reopened and found by search. Spike S6 is run here |
| 2 | `AppLifecycle`, `PlatformCaps`, `QuitPrompt.qml`, tray and `KDBusService` and `UnixSignalBridge` in `main.cpp`, `tst_applifecycle`, `single_instance_test.sh`, the `.service` file | Every lifecycle row passes in `tst_applifecycle`. `busctl --user` shows the name `io.github.rupeshofficedata.StickyNotes`. The second launch shows the first. Wayland activation token handling is checked by hand |
| 3 | `SettingsPage.qml`, `BinView.qml`, rescan and banners, color palette and dark mode, image size rules | CUJ 5, CUJ 6 and CUJ 7 pass by hand |
| 4 | `.desktop`, metainfo, icon, `PKGBUILD`, `README.md`, `LICENSE`, `src/README.md` | `makepkg` and `namcap` pass |

### Class interfaces

All calls run on the GUI thread and are synchronous.

*   **`NoteStore`:**
    *   `QString createNote()` returns the new ID.
    *   `NoteData load(id)` returns `{html, colorToken, mtimeNs, sizeBytes}`.
    *   `bool save(id, html, colorToken)` writes `meta.json` then `note.html`. On failure it emits `saveFailed(id, reason)` and returns false.
    *   `moveToBin(id)`, `bool restoreFromBin(id)`, `deleteForever(id)`, `emptyBin()`.
    *   `QString addImage(id, QImage)` returns `images/<uuid>.png`, or an empty string with an error signal.
    *   `QStringList flushAll()` returns the IDs whose save failed.
    *   `setFolder(path)`, `folder()`, `rescan()`.
    *   Signals: `saveFailed(id, reason)`, `storeUnavailable(reason)`, `noteChangedOutside(id)`, `noteRemovedOutside(id)`.
    *   Owns `QSettings` (organization `rupeshofficedata`, application `StickyNotes`) and the open-note registry.
*   **`NoteIndex`** (`QAbstractListModel`): roles `id, title, snippet, modified, colorToken, isError`. `setFilter(QString)`. A second instance, `binIndex`, lists `.bin`.
*   **`AppLifecycle`:** `newNote()`, `openNote(id)`, `showPrimary()`, `quit(force=false)`, `windowShown(kind, id)`, `windowHidden(kind, id)`, `nextCascadePosition()`. Property `trayPresent`. Signals `showPrimaryRequested()`, `openNoteRequested(id)`, `quitBlocked(QStringList ids)`. It takes a tray probe interface so tests can fake it.
*   **`RichTextBridge`:** properties `document` (`QQuickTextDocument*`), `noteId`, `bold`, `italic`, `underline`, `strike`, `bullet`. Invokables `toggleBold()`, `toggleItalic()`, `toggleUnderline()`, `toggleStrike()`, `toggleBullet()`, `pasteFromClipboard()`, `insertImageFile(QUrl)`.
*   **Flows:** open note = `load`, then bind the document. Edit = 400 ms timer, then `save`. Close = `save`, then `windowHidden`. Delete = `save`, then `moveToBin`. Restore = `restoreFromBin`, then `rescan`.

### Files to create

1.  `CMakeLists.txt`: `cmake_minimum_required(VERSION 3.21)`. `find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets DBus Qml Quick QuickControls2 QuickDialogs2 Test)`. `find_package(KF6DBusAddons REQUIRED)` and `find_package(KF6StatusNotifierItem REQUIRED)` (each framework is its own package; spike S3 confirmed this). ECM is not required because installs use `GNUInstallDirs`. Targets: static library `stickynotes_core` (NoteStore, NoteIndex, AppLifecycle, PlatformCaps), static library `stickynotes_editor` (RichTextBridge, links Qt Quick), executable `stickynotes` (`main.cpp` and `qt_add_qml_module(stickynotes URI StickyNotes VERSION 1.0 QML_FILES ...)`), tests. Install: `bin`, `share/applications`, `share/dbus-1/services`, `share/icons/hicolor/scalable/apps`, `share/metainfo`. `CMAKE_EXPORT_COMPILE_COMMANDS` is on.
2.  `.clang-tidy`: enables `bugprone-*,performance-*`.
3.  `src/main.cpp`: sets organization name `rupeshofficedata`, organization domain `rupeshofficedata.github.io`, application name `StickyNotes`, desktop file name `io.github.rupeshofficedata.StickyNotes`, and `setQuitOnLastWindowClosed(false)`. Starts `KDBusService` in Unique mode (a second launch calls `showPrimary()`). Creates the `KStatusNotifierItem` only when `PlatformCaps::trayPresent()` is true: menu "Show StickyNotes", "New note", "Quit", a left click calls `showPrimary()`, and Quit calls `AppLifecycle::quit()`. Hosts `UnixSignalBridge`. Registers the singletons and loads `qrc:/qt/qml/StickyNotes/Main.qml`. The app uses `QApplication` and links Qt Widgets, because `KStatusNotifierItem` needs them (spike S3).
4.  `src/core/NoteStore.h` and `.cpp`: the `NoteStore` interface above.
5.  `src/core/NoteIndex.h` and `.cpp`: the `NoteIndex` interface above.
6.  `src/core/AppLifecycle.h` and `.cpp`: the lifecycle table, the quit sequence and `nextCascadePosition()`.
7.  `src/core/PlatformCaps.h` and `.cpp`: `trayPresent()` (is `org.kde.StatusNotifierWatcher` registered, using `QDBusConnectionInterface::isServiceRegistered`) and `isWayland()`.
8.  `src/core/Logging.h`: `stickynotes.storage`, `stickynotes.tray`, `stickynotes.activation`.
9.  `src/editor/RichTextBridge.h` and `.cpp`: the `RichTextBridge` interface and the image rules in "Rich text and images".
10. `src/qml/Main.qml`: root item holding `PrimaryWindow` and `NoteWindowManager`.
11. `src/qml/Theme.qml`: the color token table, following `Qt.styleHints.colorScheme`.
12. `src/qml/PrimaryWindow.qml`: search box, note list, "New note" and "Refresh" buttons, Settings and Bin entries, row context menu with "Delete", the startup notice.
13. `src/qml/NoteWindow.qml`: frameless window with its own header (drag area, list button, delete button), `TextEdit` bound to a `RichTextBridge`, toolbar order bold, italic, underline, strikethrough, bullet list, color, and the banners.
14. `src/qml/NoteWindowManager.qml`: creates and tracks one `NoteWindow` per open note.
15. `src/qml/SettingsPage.qml`: notes folder setting and the "Open notes folder" button. The Change button opens `FolderDialog` from `QtQuick.Dialogs`. Validation: the folder exists, is a directory and is writable. On confirm: flush saves, close all note windows, call `NoteStore::setFolder`, rescan. The app follows the system color scheme and has no color setting.
16. `src/qml/BinView.qml`: lists binned notes with Restore, Delete forever and Empty bin. Restore needs no confirmation. Delete forever and Empty bin ask for confirmation with Cancel as the default button.
17. `src/qml/ErrorBanner.qml`: banner used by `NoteWindow.qml` and `PrimaryWindow.qml`. Takes a message and a list of action labels and emits `actionTriggered(label)`.
18. `src/qml/QuitPrompt.qml`: the Retry, Discard and quit, Cancel dialog driven by `quitBlocked`.
19. `tests/CMakeLists.txt`: registers the Qt Test executables and `single_instance_test.sh`. Tests run with `QT_QPA_PLATFORM=offscreen`.
20. `tests/tst_notestore.cpp`: save then load returns equal HTML; a second save leaves bytes unchanged; save into a read-only folder emits `saveFailed` and keeps the body; an unreadable `note.html` yields an error row; a missing notes folder emits `storeUnavailable`; bin move and restore; restore refused when the ID exists; an existing `.bin/<id>` is renamed with a suffix; empty bin; image over 10 MB rejected; non-image drop ignored; image link outside `images/` not loaded; remote URL not loaded; invalid `meta.json` falls back to `default`; `flushAll` returns failed IDs; the four external-change cells (clean or unsaved, changed or removed); an outside color change; retry after 10 seconds with a fake clock; `QSettings` entries removed on delete forever; a default folder is created and a missing user-chosen folder is not.
21. `tests/tst_noteindex.cpp`: case-insensitive substring filter; the Bin is not searched; sort by modified time, newest first; title is the first non-empty line cut to 60 characters, "Untitled" for an empty note; a note never opened is found by search; rescan adds, changes and removes one row without resetting the model; 1,000 notes load.
22. `tests/tst_applifecycle.cpp`: every row of the lifecycle table with a fake tray probe (tray present or not; primary visible or hidden; last note closed; minimized counts as visible); a failed save emits `quitBlocked`; `quit(force=true)` skips the prompt; cascade positions wrap after 8 notes.
23. `tests/tst_richtextbridge.cpp`: bold, italic, underline, strikethrough and bullet list survive save and reload; image insert; remote and out-of-folder links are not loaded; text paste is plain text.
24. `tests/single_instance_test.sh`: with `dbus-run-session` and the offscreen platform, start the app, wait until `NameHasOwner` reports `io.github.rupeshofficedata.StickyNotes`, start a second instance, check that it exits with status 0 and that the first instance's log shows `showPrimary`.
25. `packaging/io.github.rupeshofficedata.StickyNotes.desktop`: `Type=Application`, `Name=StickyNotes`, `Exec=stickynotes`, `Icon=io.github.rupeshofficedata.StickyNotes`, `Categories=Utility;`, `StartupNotify=true`, `StartupWMClass=stickynotes`, `DBusActivatable=true`.
26. `packaging/io.github.rupeshofficedata.StickyNotes.service`: `Name=io.github.rupeshofficedata.StickyNotes`, `Exec=/usr/bin/stickynotes`.
27. `packaging/io.github.rupeshofficedata.StickyNotes.metainfo.xml`: AppStream ID, name, summary, description, GPL-3.0-or-later license, launchable pointing at the `.desktop` file, a release entry and screenshots.
28. `packaging/io.github.rupeshofficedata.StickyNotes.svg`: scalable app icon (a simple note square).
29. `packaging/PKGBUILD`: `pkgname=stickynotes`, `pkgver`, `pkgrel`, `pkgdesc`, `arch=(x86_64)`, `url`, `license=(GPL-3.0-or-later)`, `depends=(qt6-base qt6-declarative kdbusaddons kstatusnotifieritem)` (names confirmed in the Arch `extra` repository), `makedepends=(cmake)`, `source`, `sha256sums`, `build()` (cmake configure and build), `package()` (cmake install with `DESTDIR`).
30. `README.md`: install, notes location, folder change, synced folders, desktop support, KWin always-on-top rule, and a bug-report checklist (distro, desktop, app version, Qt version, `QT_LOGGING_RULES` output).
31. `LICENSE`: the full text of the GNU GPL version 3.
32. `src/README.md`: written at the end of Stage 4. Describes the interface, invariants and side effects of each source directory.

### Verification Plan

*   **Build**: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build`
*   **Unit tests**: `ctest --test-dir build --output-on-failure`
*   **Static analysis**: `clang-tidy -p build src/core/*.cpp src/editor/*.cpp`. Pass means zero warnings. `qmllint -I build src/qml/*.qml` is non-gating if singleton imports cannot be resolved.
*   **Packaging checks**: `appstreamcli validate --no-net packaging/*.metainfo.xml`, `desktop-file-validate packaging/*.desktop`, `makepkg` and `namcap`.
*   **Test coverage**: each data-loss path in the Risks table has at least one test.
*   **Manual checks**: walk CUJ 1 to CUJ 7 on Plasma 6.4+ Wayland, and CUJ 1 to CUJ 3 on one GNOME and one Sway session (pass means no crash and the CUJ completes). External-change test: edit `note.html` outside the app, focus the note window, and check that the note reloads or shows the banner.

--------------------------------------------------------------------------------

## Implementation status (2026-10-28)

Stages 0 to 4 are built. The code is in `src/`, the tests in `tests/`, the packaging in `packaging/`.

*   **Tests:** 6 test programs pass (`ctest --test-dir build`): `NoteStore` (23 cases), `NoteIndex` (8), `RichTextBridge` (6), `AppLifecycle` (14), a headless run of the real `NoteWindow.qml` (9 cases: typing, autosave, close and reopen, the outside-change cells, remote image filter), and the single-instance script.
*   **Static checks:** `clang-tidy` on `src/core` and `src/editor` reports zero warnings. `qmllint` is clean. `desktop-file-validate` and `appstreamcli validate --no-net` pass. A local `makepkg` build produced a package with the expected files.
*   **Not checked by a person yet:** the full flow in the real GUI (clicks, paste from Spectacle, Firefox and Dolphin, drag and drop), the tray menu, the quit prompt, the folder dialog, and Wayland activation tokens. `namcap` is not installed on the development machine.
*   **Differences from this document:** `NoteStore` reports the modified time as `mtimeMs` (milliseconds). The "removed outside" case on a note with no unsaved edits closes the window without showing a message. `PKGBUILD` assumes a GitHub repository `rupeshofficedata/StickyNotes` with a `v0.1.0` tag, which does not exist yet.
*   **Not built:** nothing from the Future Improvements list.

--------------------------------------------------------------------------------

*Drafted with EGM*
