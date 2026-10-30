# Goldfish Annotated Review Report

**Target:** `docs/designs/sticky_notes.md` (lines 316-374, Detailed Implementation Plan; the feature spec above is context)
**Source findings:** `implementation_readiness_report.md` (M1 to M10, N1 to N15) and four extra points (R1 to R4) from a second run of the readiness reviewer.
**Status:** resolved (auto accept: 37 PROPOSE FIX applied, 3 DECLINE skipped, 1 ESCALATE answered: keep Plasma 6.4+)

**Summary:** 41 items: 37 PROPOSE FIX, 3 DECLINE, 1 ESCALATE, 0 CONTEST. Issues 1 to 29 come from the consolidated report and the second readiness run. Issues 30 to 41 come from the codebase-details reviewer (see Addendum). No finding is contested. Each one points at something the document leaves to the implementer.

**Evidence gathered while triaging (this machine, CachyOS):**
* `pacman -Si` finds `qt6-base`, `qt6-declarative`, `qt6-svg`, `kdbusaddons`, `kstatusnotifieritem` and `extra-cmake-modules` in the `extra` repository. Versions: Qt 6.12.0, KDE Frameworks 6.31.0.
* The CMake package names are `KF6DBusAddons` and `KF6StatusNotifierItem` (from the files installed under `/usr/lib/cmake/`).
* This machine has Qt 6.12, so it cannot show that the app builds at the Qt 6.5 floor. Spike S3 must test the floor.

## Prioritized Findings & AI Suggestions

### [Issue 1] M1 `[implementation-readiness, style]` - Severity: High
*   **Affected Scope:** Lines 318, 324 (Detailed Implementation Plan intro, Stage order)
*   **Discrepancy Description:** The plan says Qt 6.5 is "confirmed by S1 and S3". The spec says it is provisional, and the spikes have not run. Stage 0 has no deliverable or fallback.
*   **Rationale:** The implementer must choose the format and the Qt version after S1.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. The claim is false until the spikes run.
*   ***Proposed Edit:***
    *   Location: line 318 and Stage 0
    *   Change: Replace "Qt minimum 6.5 (confirmed by S1 and S3)" with "Qt minimum 6.5 (provisional until spikes S1 and S3 pass; see Spikes)". Replace Stage 0 with: "Stage 0: run spikes S1 and S3 as throwaway programs in `spikes/` (not part of the build). Record each result in the Spikes table. Exit check: both rows say pass or name the fallback taken. If S1 selects Markdown, change these items before Stage 1: the note file name and extension, the `NoteStore` serializer, `tst_notestore.cpp`, `tst_richtextbridge.cpp`, and the spec lines that mention underline."

### [Issue 2] M2 `[implementation-readiness, style]` - Severity: High
*   **Affected Scope:** Lines 322-361 (Stage order, file list)
*   **Discrepancy Description:** The 29 files are not assigned to stages. Stages have no exit checks. Image paste and drop appear in Stage 1 and Stage 3.
*   **Rationale:** Without a map the implementer picks the build order.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Stage order"
    *   Change: Replace the list with a table.
        *   **Stage 0:** spikes. Exit: Spikes table filled.
        *   **Stage 1:** `CMakeLists.txt`, `main.cpp` (no tray, no D-Bus yet), `NoteStore`, `NoteIndex`, `Logging.h`, `RichTextBridge` (formatting, paste, drop), `Theme.qml`, `PrimaryWindow.qml`, `NoteWindow.qml`, `NoteWindowManager.qml`, `ErrorBanner.qml`, `tests/CMakeLists.txt`, `tst_notestore`, `tst_noteindex`, `tst_richtextbridge`. Exit: build passes, those tests pass, a note can be created, edited, closed, reopened and found by search.
        *   **Stage 2:** `AppLifecycle`, `PlatformCaps`, `QuitPrompt.qml`, tray and `KDBusService` in `main.cpp`, `UnixSignalBridge` in `main.cpp`, `tst_applifecycle`, `single_instance_test.sh`, the `.service` file. Exit: every lifecycle row passes, second launch shows the first.
        *   **Stage 3:** `SettingsPage`, `BinView`, focus rescan and banners, dark mode and color picker. Exit: CUJ 5, CUJ 6 and CUJ 7 pass by hand.
        *   **Stage 4:** `.desktop`, metainfo, icon, `PKGBUILD`, `README.md`, `LICENSE`, `src/README.md`. Exit: `makepkg` and `namcap` pass.
    *   Rename Stage 3's image item to "image size rules and limits" since paste and drop belong to Stage 1.

### [Issue 3] M3 `[implementation-readiness, style]` - Severity: High
*   **Affected Scope:** Lines 332-361 (core class entries)
*   **Discrepancy Description:** Public APIs are named but not specified: no methods, arguments, model roles, properties, or call order.
*   **Rationale:** Tests cannot be written without them.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. This is the largest gap.
*   ***Proposed Edit:***
    *   Location: after the file list, new subsection "Class interfaces"
    *   Change: Add the following.
        *   **`NoteStore` (GUI thread, synchronous):** `QString createNote()` returns the new ID. `NoteData load(id)` returns `{html, colorToken, mtimeNs, sizeBytes}`. `bool save(id, html, colorToken)` writes `meta.json` then `note.html`, each through `QSaveFile`, and on failure emits `saveFailed(id, reason)` and returns false. `moveToBin(id)`, `bool restoreFromBin(id)`, `deleteForever(id)`, `emptyBin()`. `QString addImage(id, QImage)` returns `images/<uuid>.png` or an empty string with an error signal. `QStringList flushAll()` returns the IDs that failed. `setFolder(path)` and `folder()`. Signals: `saveFailed`, `storeUnavailable`, `noteChangedOutside(id)`, `noteRemovedOutside(id)`.
        *   **`NoteIndex` (`QAbstractListModel`):** roles `id, title, snippet, modified, colorToken, isError`. `setFilter(QString)`, `rescan()`. A second instance, `binIndex`, lists `.bin`.
        *   **`AppLifecycle`:** `newNote()`, `openNote(id)`, `showPrimary()`, `quit()`, plus `windowShown(kind, id)` and `windowHidden(kind, id)` called by QML windows from `onVisibleChanged`. Property `trayPresent`. Signals `showPrimaryRequested()` and `quitBlocked(QStringList ids)`.
        *   **`RichTextBridge`:** properties `document` (`QQuickTextDocument*`), `noteId`, `bold`, `italic`, `underline`, `strike`, `bullet`. Invokables `toggleBold()`, `toggleItalic()`, `toggleUnderline()`, `toggleStrike()`, `toggleBullet()`, `pasteFromClipboard()`, `insertImageFile(QUrl)`.
        *   **Flows (call order):** open note = `load` then bind the document; edit = 400 ms timer then `save`; close = `save` then `windowHidden`; delete = `save` then `moveToBin`; restore = `restoreFromBin` then `NoteIndex::rescan`.

### [Issue 4] M4 `[implementation-readiness]` - Severity: High
*   **Affected Scope:** Lines 333, 336 (`main.cpp`, `AppLifecycle`)
*   **Discrepancy Description:** SIGTERM handling, save flush and the quit prompt have no mechanism or owner.
*   **Rationale:** This is the data-loss path the design cares about most.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `main.cpp` and `AppLifecycle` entries, and the file list
    *   Change: `main.cpp` contains a small class `UnixSignalBridge` (a socket pair read by a `QSocketNotifier`) that calls `AppLifecycle::quit(force=true)` on SIGTERM and SIGINT. `AppLifecycle::quit()` calls `NoteStore::flushAll()`. If it returns IDs, the lifecycle emits `quitBlocked(ids)` and does not quit. `QuitPrompt.qml` (new file) shows Retry, Discard and quit, or Cancel. `quit(force=true)` skips the prompt, so a SIGTERM with a failed save loses the unsaved text, as the spec says. `aboutToQuit` calls `flushAll()` once more.

### [Issue 5] M5 `[implementation-readiness]` - Severity: High
*   **Affected Scope:** Lines 334, 335 (`NoteStore`, `NoteIndex`; spec lines 197-206)
*   **Discrepancy Description:** "Newer" is undefined, own writes cannot be told from outside edits, and nothing owns the open-note registry.
*   **Rationale:** A wrong rule either loses outside edits or reloads over the user's own save.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Core data-loss path.
*   ***Proposed Edit:***
    *   Location: new subsection "Rescan algorithm" after "Class interfaces"
    *   Change: `NoteStore` owns the open-note registry: `{id, recordedMtimeNs, recordedSizeBytes, dirty}`. It records `mtimeNs` and `sizeBytes` of `note.html` after every load and every successful save. `rescan()` runs on the GUI thread when the application becomes active (`applicationStateChanged` to `Qt::ApplicationActive`) and on Refresh. For each open note it stats `note.html`: missing = `noteRemovedOutside(id)`; any difference from the recorded pair = `noteChangedOutside(id)`. For closed notes it compares with the index cache and updates rows. A note that fails to parse stays an error row and is tried again on the next rescan. There is no debounce, since the trigger is a user action.

### [Issue 6] M6 `[implementation-readiness]` - Severity: High
*   **Affected Scope:** Lines 332, 336, 339 (`CMakeLists.txt`, layering)
*   **Discrepancy Description:** The build file is one sentence. `AppLifecycle` sits in a core library with no Qt Quick but controls QML windows.
*   **Rationale:** Core cannot link Qt Quick, so the lifecycle boundary is a real gap.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `CMakeLists.txt` entry and "Class interfaces"
    *   Change: `cmake_minimum_required(VERSION 3.21)`. `find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui DBus Qml Quick QuickControls2 QuickDialogs2 Test)`. `find_package(KF6 REQUIRED COMPONENTS DBusAddons StatusNotifierItem)` (the config packages `KF6DBusAddons` and `KF6StatusNotifierItem` exist on this machine). ECM is not required because installs use `GNUInstallDirs`. Targets: static library `stickynotes_core` (NoteStore, NoteIndex, AppLifecycle, PlatformCaps), executable `stickynotes` (main, RichTextBridge, QML module via `qt_add_qml_module(stickynotes URI StickyNotes VERSION 1.0 QML_FILES ...)`), and tests linked to `stickynotes_core` plus the sources they need. Singletons are registered in `main.cpp` with `qmlRegisterSingletonInstance`. The core never calls QML: `AppLifecycle` learns window visibility from `windowShown` and `windowHidden` and asks QML to show windows through the `showPrimaryRequested()` signal. Install destinations: `bin`, `share/applications`, `share/dbus-1/services`, `share/icons/hicolor/scalable/apps`, `share/metainfo`.

### [Issue 7] M7 `[implementation-readiness]` - Severity: High
*   **Affected Scope:** Line 357 (`PKGBUILD`)
*   **Discrepancy Description:** The plan tells the implementer to confirm package names. `makedepends`, source, checksums and build functions are absent.
*   **Rationale:** An open lookup breaks the zero-decision rule.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. The lookup is done: all five names exist in `extra` (see Evidence above).
*   ***Proposed Edit:***
    *   Location: `PKGBUILD` entry and Verification
    *   Change: "`depends=(qt6-base qt6-declarative qt6-svg kdbusaddons kstatusnotifieritem)` (names confirmed in the Arch `extra` repository). `makedepends=(cmake)`. Fields: `pkgname=stickynotes`, `pkgver`, `pkgrel`, `pkgdesc`, `arch=(x86_64)`, `url`, `license=(GPL-3.0-or-later)`, `source`, `sha256sums`, `build()` (cmake configure and build), `package()` (cmake install with DESTDIR). Verification adds `makepkg` and `namcap`." Drop `qt6-svg` if the icon is only installed as a file and never loaded by Qt.

### [Issue 8] M8 `[implementation-readiness, codebase-details]` - Severity: High
*   **Affected Scope:** Lines 333, 337, 348, 353-354 (tray, D-Bus files, ID conflict, delete forever)
*   **Discrepancy Description:** The tray check, tray menu, `.service` and `.desktop` keys, the restore conflict rule and `meta.json` failure have no specification.
*   **Rationale:** Tests and the lifecycle table assert behavior the document never defines.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Note on disk", `PlatformCaps`, packaging entries
    *   Change:
        *   **Tray probe:** `PlatformCaps::trayPresent()` returns true when `org.kde.StatusNotifierWatcher` is registered on the session bus (`QDBusConnectionInterface::isServiceRegistered`).
        *   **Tray item:** `main.cpp` creates a `KStatusNotifierItem` only when the probe is true. Menu: "Show StickyNotes", "New note", "Quit". A left click calls `showPrimary()`.
        *   **`.desktop` keys:** `Type=Application`, `Name=StickyNotes`, `Exec=stickynotes`, `Icon=io.github.rupeshkumar.StickyNotes`, `Categories=Utility;`, `StartupNotify=true`, `StartupWMClass=stickynotes`, `DBusActivatable=true`.
        *   **`.service` keys:** `Name=io.github.rupeshkumar.StickyNotes`, `Exec=/usr/bin/stickynotes`.
        *   **Restore conflict:** If `<notes>/<id>` already exists, restore is refused. The note stays in the bin and the UI says "A note with this ID already exists."
        *   **Delete forever:** remove `.bin/<id>`, then remove the note's `QSettings` entries.
        *   **`meta.json` unreadable or with an unknown color token:** the note loads with the `default` token.

### [Issue 9] M9 `[implementation-readiness, style]` - Severity: High
*   **Affected Scope:** Lines 347-352, 367 (tests)
*   **Discrepancy Description:** Tests name areas, not cases. Several spec requirements have no test. The lifecycle test needs a seam for the tray and windows.
*   **Rationale:** Tests cannot be written to a topic list.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: test entries and Verification
    *   Change: Replace the one-line entries with case lists. All tests run with `QT_QPA_PLATFORM=offscreen`. `AppLifecycle` takes a `TrayProbe` and reports window visibility through `windowShown` and `windowHidden`, so tests use a fake probe and call those methods.
        *   **`tst_notestore`:** save then load returns equal HTML; second save leaves bytes unchanged; save into a read-only folder emits `saveFailed` and keeps the body; unreadable `note.html` yields an error row; missing notes folder emits `storeUnavailable`; bin move and restore; restore refused when the ID exists; empty bin; image over 10 MB rejected; non-image drop ignored; image link outside `images/` not loaded; remote URL not loaded; `meta.json` invalid falls back to `default`; `flushAll` returns failed IDs.
        *   **`tst_noteindex`:** substring filter is case-insensitive; Bin excluded from search; sort by modified time, newest first; title is the first non-empty line cut to 60 characters, "Untitled" for an empty note; rescan adds, changes and removes one row without resetting the model.
        *   **`tst_applifecycle`:** every row of the lifecycle table (with and without tray; primary visible or hidden; last note closed); quit with a failed save emits `quitBlocked`; `quit(force=true)` skips the prompt.
        *   **`tst_richtextbridge`:** bold, italic, underline, strikethrough and bullet survive save and reload; image insert; cascade wraps after 8 notes (positions are computed in the `NoteWindowManager`'s C++ helper or tested by hand).
        *   **External-change table:** four cells (clean or dirty, changed or removed) tested through `NoteStore` registry signals.
        *   **Retry:** a failed save is retried after 10 seconds with a fake clock.
        *   **`single_instance_test.sh`:** start the app, wait until `org.freedesktop.DBus.NameHasOwner` reports `io.github.rupeshkumar.StickyNotes`, start a second instance, check that it exits with status 0 and that the first instance's log shows `showPrimary`.
    *   Replace the coverage bullet with: "Each data-loss path in the Risks table has at least one test."

### [Issue 10] M10 `[implementation-readiness, style]` - Severity: High
*   **Affected Scope:** Lines 364-366 (Verification)
*   **Discrepancy Description:** `clang-tidy` and `qmllint` commands lack flags and a pass rule.
*   **Rationale:** An implementer cannot tell pass from fail.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: Verification Plan
    *   Change: Build with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`. Run `clang-tidy -p build src/core/*.cpp src/editor/*.cpp` with a `.clang-tidy` file enabling `bugprone-*,performance-*` (add the file to the list). Pass = zero warnings. Run `qmllint -I build src/qml/*.qml`. qmllint is non-gating if singleton imports cannot be resolved. Add `makepkg` and `namcap` for the AUR package.

### [Issue 11] R1 `[implementation-readiness, second run]` - Severity: Medium
*   **Affected Scope:** `NoteStore::save`, "Removed outside" row of the external-change table
*   **Discrepancy Description:** Nothing says whether `meta.json` and `images/` are recreated when an open note was removed outside and the user chooses Save. The save order of `meta.json` and `note.html` is not stated.
*   **Rationale:** A crash between the two writes, or a recreated note with missing files, are data-loss edge cases.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Saving" and the external-change table
    *   Change: "`save` writes `meta.json` first and `note.html` second, each through `QSaveFile`. A crash between the two leaves the old text with the new color, which is harmless. Saving a note whose folder was removed outside recreates the folder, `meta.json` and `note.html`. Image links to missing files show the placeholder."

### [Issue 12] R2 `[implementation-readiness, second run]` - Severity: Medium
*   **Affected Scope:** `SettingsPage.qml` (line 344), CUJ 5
*   **Discrepancy Description:** The folder dialog, the validation order and what happens to open notes on a switch are undefined.
*   **Rationale:** CUJ 5 says "confirms" but defines no confirmation.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `SettingsPage.qml` entry
    *   Change: "The Change button opens `FolderDialog` from `QtQuick.Dialogs` (needs the `QuickDialogs2` module). Validation order: the folder exists, is a directory and is writable. On confirm, the app flushes saves, closes all note windows, calls `NoteStore::setFolder`, and rescans. A confirmation line states that no notes are copied or moved."

### [Issue 13] R3 `[implementation-readiness, second run]` - Severity: Medium
*   **Affected Scope:** `NoteStore::moveToBin`, `BinView.qml`
*   **Discrepancy Description:** The behavior when `.bin/<id>` already exists is undefined.
*   **Rationale:** Overwriting would lose a note.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Note on disk", delete section
    *   Change: "If `.bin/<id>` already exists, the older entry is renamed to `.bin/<id>.<unix-ms>` before the move. Entries with a suffix are listed in the Bin like any other."

### [Issue 14] R4 `[implementation-readiness, second run]` - Severity: Medium
*   **Affected Scope:** Spec "Notes changed by outside tools" (lines 197-206)
*   **Discrepancy Description:** "Keep mine" and "Reload" semantics: what the banner compares against.
*   **Rationale:** Covered by Issue 5, which defines the recorded pair.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Folded into Issue 5. Add one sentence to the spec: "Keep mine updates the recorded pair to the current file, so the banner returns only if the file changes again. Reload replaces the text and discards unsaved edits."
*   ***Proposed Edit:***
    *   Location: spec table "Notes changed by outside tools"
    *   Change: add the sentence above.

### [Issue 15] N1 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Lines 341-343, 236
*   **Discrepancy Description:** UI details and the GNOME fallback header trigger are undefined.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. The simplest rule removes the detection problem.
*   ***Proposed Edit:***
    *   Location: "Platform differences" and `NoteWindow.qml`
    *   Change: "Note windows always draw their own header (drag area, list button, delete button), so no server-side-decoration detection is needed." Toolbar order: bold, italic, underline, strikethrough, bullet list, color, then delete in the header. `NoteWindowManager` is created in `main.cpp` as a child of the application object so it lives while the primary window is hidden.

### [Issue 16] N2 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Line 345 (`BinView.qml`)
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `BinView.qml`
    *   Change: "Restore needs no confirmation. Delete forever and Empty bin ask for confirmation. Default button is Cancel. Delete forever removes the folder, then the `QSettings` entries."

### [Issue 17] N3 `[implementation-readiness, style]` - Severity: Low
*   **Affected Scope:** Lines 358-360
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `README.md`, `src/README.md`
    *   Change: README: "Install, notes location, folder change, synced folders, desktop support, KWin always-on-top rule, bug-report checklist (distro, desktop, app version, Qt version, `QT_LOGGING_RULES` output)". `src/README.md` belongs to Stage 4 and is written at its end.

### [Issue 18] N4 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Line 359 (`LICENSE`)
*   ***AI Suggestion:*** DECLINE
*   ***Justification:*** Nit. Copying the text from gnu.org is obvious to an implementer.
*   ***Proposed Edit:***
    *   Location: none
    *   Change: none

### [Issue 19] N5 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Lines 333, 338
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. `QSettings` depends on the names.
*   ***Proposed Edit:***
    *   Location: `main.cpp` and `Logging.h`
    *   Change: `main.cpp` calls `setOrganizationName("rupeshkumar")`, `setApplicationName("StickyNotes")`, `setQuitOnLastWindowClosed(false)`, and loads the QML module URL `qrc:/qt/qml/StickyNotes/PrimaryWindow.qml`. Log categories: `stickynotes.storage`, `stickynotes.tray`, `stickynotes.activation`.

### [Issue 20] N6 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Line 357
*   ***AI Suggestion:*** DECLINE
*   ***Justification:*** Nit. There are no optional dependencies in v1.
*   ***Proposed Edit:***
    *   Location: none
    *   Change: none

### [Issue 21] N7 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Lines 353-356
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Covered by Issue 8 and Issue 6, which list the keys and install destinations.
*   ***Proposed Edit:***
    *   Location: packaging entries
    *   Change: see Issues 6 and 8.

### [Issue 22] N8 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Line 318
*   ***AI Suggestion:*** DECLINE
*   ***Justification:*** Nit. C++17 is supported by every compiler that builds Qt 6.
*   ***Proposed Edit:***
    *   Location: none
    *   Change: none

### [Issue 23] N9 `[implementation-readiness, style]` - Severity: Low
*   **Affected Scope:** Line 344
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. The spec has no color-scheme setting.
*   ***Proposed Edit:***
    *   Location: `SettingsPage.qml`
    *   Change: "Notes folder setting, 'Open notes folder' button. The app follows the system color scheme and has no color setting."

### [Issue 24] N10 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Line 346
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: `ErrorBanner.qml`
    *   Change: "Banner used by `NoteWindow.qml` and `PrimaryWindow.qml`. Takes a message and a list of action labels, and emits `actionTriggered(label)` to the caller."

### [Issue 25] N11 `[implementation-readiness, style]` - Severity: Low
*   **Affected Scope:** Lines 330, 332-360
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: heading and entries
    *   Change: Rename the heading "Files to create". State "All files are new" once. Drop "(Create)" from each entry.

### [Issue 26] N12 `[implementation-readiness, style]` - Severity: Low
*   **Affected Scope:** Lines 318, 325, 336
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: intro paragraph
    *   Change: Rewrite as a list: Name `StickyNotes`; App ID; License; Language C++17 and QML; Build CMake; Qt minimum 6.5 (provisional); KDE Frameworks 6 components `DBusAddons` and `StatusNotifierItem`. Add "(see Spikes)" and "(see Lifecycle)" where S1, S3, S6 and the lifecycle table are mentioned. Use "KStatusNotifierItem" for the class and "StatusNotifierItem" for the CMake component, always in code font.

### [Issue 27] N13 `[implementation-readiness, style]` - Severity: Low
*   **Affected Scope:** Lines 348, 351, 367, 368
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Most is covered by Issue 9.
*   ***Proposed Edit:***
    *   Location: tests and Verification
    *   Change: "ID conflict" instead of "ID clash"; "survive save and reload" instead of "round-trip"; rewrite the manual-check bullet as "Manual checks: walk CUJ 1 to CUJ 7 on Plasma 6.4+ Wayland, and CUJ 1 to CUJ 3 on one GNOME and one Sway session. External-change test: edit `note.html` outside the app, focus the note window, and check that the note reloads or shows the banner."

### [Issue 28] N14 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Lines 328, 372, 374
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid and cheap.
*   ***Proposed Edit:***
    *   Location: end of file
    *   Change: Keep the footer "*Drafted with EGM*" and remove the HTML provenance comment.

### [Issue 29] N15 `[implementation-readiness]` - Severity: Low
*   **Affected Scope:** Lines 13, 62 (Plasma 6.4 floor)
*   **Discrepancy Description:** The Plasma 6.4 floor has no stated reason.
*   **Rationale:** The floor came from KWin's session-restore support in Plasma 6.4. Session restore is no longer in v1, so the reason is gone.
*   ***AI Suggestion:*** ESCALATE
*   ***Justification:*** The Plasma floor is a support decision that only the owner can make. The document holds no evidence either way. Options: keep "Plasma 6.4+" as the tested floor, relax it to "Plasma 6", or state "developed and tested on Plasma 6.x current".
*   ***Proposed Edit:***
    *   Location: Overview, Goals ("Plasma first")
    *   Change: Wait for the owner's answer.

## Unverified claims (no edit proposed; spikes or manual checks cover them)

* `Qt.styleHints.colorScheme` and the QtCore `Settings` QML type at Qt 6.5 (covered by S1).
* `KDBusService` with `DBusActivatable=true` and the second launch raising the first (covered by `single_instance_test.sh`).
* `QSaveFile` rename behavior on synced folders and across filesystems.
* `startSystemMove()` and `startSystemResize()` on frameless QML windows under KWin.
* The app building at the Qt 6.5 floor (this machine has 6.12; S3 must test the floor).

## Addendum: findings from the codebase-details reviewer (arrived after the sections above)

The codebase-details reviewer (3 Blockers, 12 Majors, 8 Nits, 14 claims to verify) finished after the report above was written. Most of its findings repeat Issues 1 to 14. The ones below are new or change an earlier proposal. All are PROPOSE FIX.

### [Issue 30] A1 `[codebase-details]` - Severity: High
*   **Affected Scope:** Spec lines 116, 144, 212, 225 (settings)
*   **Discrepancy Description:** Per-note sizes keyed by note ID, with removal on delete, do not fit the QtCore `Settings` QML type, which uses declared properties. `NoteStore` needs the notes-folder path in C++ before QML loads.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. This changes the earlier choice of the QML `Settings` type.
*   ***Proposed Edit:***
    *   Location: "C++ and QML interface", Open Questions, spike S1
    *   Change: `NoteStore` owns a C++ `QSettings` for the folder path and per-note sizes. Drop the QtCore `Settings` QML type. Remove "Does the QtCore Settings QML type work" from spike S1. Change the Qt floor note from "`Qt.styleHints.colorScheme` API and the QtCore `Settings` QML type" to "`Qt.styleHints.colorScheme` API".

### [Issue 31] A2 `[codebase-details]` - Severity: High
*   **Affected Scope:** Lines 133-139, 221, 332-337 (layering, tray)
*   **Discrepancy Description:** No file creates the tray item. `KStatusNotifierItem` has a menu and may need Qt Widgets. The default tray Quit and the session-logout path can bypass `AppLifecycle`.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Extends Issue 8.
*   ***Proposed Edit:***
    *   Location: `main.cpp`, `AppLifecycle`
    *   Change: The app target owns the `KStatusNotifierItem` (created in `main.cpp`). It passes `trayPresent` to `AppLifecycle`. The item's Quit action is overridden to call `AppLifecycle::quit()`. The app links Qt Widgets only if the S3 build check shows `KStatusNotifierItem` needs it. `QGuiApplication::commitDataRequest` calls `quit(force=true)` so a logout flushes saves.

### [Issue 32] A3 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 50, 246-249 (spike S3)
*   **Discrepancy Description:** S3 is a Flatpak spike in the Stage 0 gate, but a Flatpak is a first-release non-goal. The S3 failure row does not say what happens to the Qt floor.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: Spikes table
    *   Change: Rename S3 "Qt floor and KF6 build check". Pass: the app skeleton builds against Qt 6.5 and the KF6 components. Fail: raise the Qt floor to the lowest version that builds. Move the Flatpak runtime and portal checks to Future Improvements under Flathub.

### [Issue 33] A4 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 229-231 (search)
*   **Discrepancy Description:** Search covers "loaded" notes, but a note is parsed only when shown, so unopened notes are never searched. "No specific limit" is not a decision.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Search and list"
    *   Change: `NoteIndex` reads the title and plain text of every note at startup and again whenever a note's modification time changes. Tested with 1,000 notes. No larger ceiling is promised.

### [Issue 34] A5 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 151, 199-206 (rescan)
*   **Discrepancy Description:** A color change touches only `meta.json`, so rescan ignores outside color edits. Own writes change the modification time.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Extends Issue 5.
*   ***Proposed Edit:***
    *   Location: "Rescan algorithm"
    *   Change: Record and compare `(mtime, size)` for both `note.html` and `meta.json`. An outside color change updates the color on a clean open note. List order still uses the modification time of `note.html`.

### [Issue 35] A6 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 26, 64, 195 (folder creation)
*   **Discrepancy Description:** CUJ 1 creates the default folder, but the spec says the app never creates an empty store for a missing folder. `QStandardPaths::AppDataLocation` does not give the app ID.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid.
*   ***Proposed Edit:***
    *   Location: "Folder change" and CUJ 1
    *   Change: The app creates the folder only when its path equals the default and it does not exist. A user-chosen folder that is missing is an error. Default path: `QStandardPaths::GenericDataLocation` + `/io.github.rupeshkumar.StickyNotes/notes` (falls back to `~/.local/share` when `XDG_DATA_HOME` is unset).

### [Issue 36] A7 `[codebase-details]` - Severity: High
*   **Affected Scope:** Lines 150-153, 214-217 (rich text and images)
*   **Discrepancy Description:** "Never loads a remote URL" has no mechanism. The document base URL, text paste policy, image display scale and what the 10 MB cap measures are undefined. A pasted dark foreground on a near-black note is unreadable.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. This is a security and readability boundary.
*   ***Proposed Edit:***
    *   Location: "Rich text and images"
    *   Change: `RichTextBridge` sets the document base URL to the note folder and filters `img src` values on load so that only `images/<uuid>.png` inside the note folder loads. Text paste is plain text. The 10 MB cap measures the source bytes. Non-PNG images are re-encoded to PNG, and animated GIFs lose animation. Images display scaled down to the window width. The byte-identical second save is S1's test, not a stated property.

### [Issue 37] A8 `[codebase-details]` - Severity: High
*   **Affected Scope:** Lines 130, 172, 222, 343 (window ownership)
*   **Discrepancy Description:** Who instantiates `NoteWindowManager.qml` and how `AppLifecycle.openNote(id)` reaches it is undefined. Whether a minimized window counts as visible is not stated.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Extends Issue 3.
*   ***Proposed Edit:***
    *   Location: "C++ and QML interface"
    *   Change: A root QML file `Main.qml` (not a window) holds `PrimaryWindow` and `NoteWindowManager`, so the manager lives while the primary window is hidden. `AppLifecycle.openNote(id)` emits `openNoteRequested(id)`, and the manager creates the `NoteWindow`. Each window reports `windowShown` and `windowHidden`. A minimized window counts as visible.

### [Issue 38] A9 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 31, 88, 96-100, 341-346 (delete and folder-switch flows)
*   **Discrepancy Description:** No Delete control is described for the primary list. The window and pending save of a note deleted while open are unspecified.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Extends Issues 12 and 15.
*   ***Proposed Edit:***
    *   Location: `PrimaryWindow.qml`, `NoteWindow.qml`
    *   Change: Delete is a button in the note window header and a "Delete" entry in the context menu of each list row. Deleting an open note saves it, moves it to the bin, and closes its window.

### [Issue 39] A10 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 119, 333, 353-354 (D-Bus identity)
*   **Discrepancy Description:** `KDBusService` builds its D-Bus name from the organization domain and application name. The plan sets neither. The executable name is not stated.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Extends Issue 8.
*   ***Proposed Edit:***
    *   Location: `main.cpp`, packaging entries
    *   Change: `main.cpp` sets `organizationDomain` to `rupeshkumar.github.io`, `applicationName` to `StickyNotes`, and `desktopFileName` to `io.github.rupeshkumar.StickyNotes`. The executable is `stickynotes`. Stage 2 verifies with `busctl --user` that the registered name equals the app ID. The Wayland activation token handling is also checked in Stage 2.

### [Issue 40] A11 `[codebase-details]` - Severity: Medium
*   **Affected Scope:** Lines 122, 179, 186-189 (quit flow)
*   **Discrepancy Description:** A dialog cannot run inside `aboutToQuit`. Cancel must abort a quit that began from the tray.
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid. Adds detail to Issue 4.
*   ***Proposed Edit:***
    *   Location: "Saving"
    *   Change: Sequence: `AppLifecycle::quit()` calls `flushAll()`. On failures it emits `quitBlocked(ids)` and returns. `QuitPrompt.qml` shows Retry (calls `quit()` again), Discard and quit (calls `quit(force=true)`) or Cancel (does nothing). `aboutToQuit` runs a final `flushAll()` and never opens a dialog.

### [Issue 41] A12 `[codebase-details]` - Severity: Low
*   **Affected Scope:** Lines 128-145, 155, 210, 345, 366 (small items)
*   ***AI Suggestion:*** PROPOSE FIX
*   ***Justification:*** Valid and cheap.
*   ***Proposed Edit:***
    *   Location: diagram, spec, Verification
    *   Change: Add `Theme.qml`, `ErrorBanner.qml`, `QuitPrompt.qml` and `Logging.h` to the Components diagram. The cascade counts notes opened in the current session. Clicking an error row shows the reason. `appstreamcli validate --no-net packaging/*.metainfo.xml`. The GNOME and Sway pass criterion is that CUJ 1 to CUJ 3 complete with no crash.

## Additional claims to verify (from codebase-details; no edit proposed)

* `Qt.styleHints` versus `Application.styleHints` in QML, and the version `colorScheme` appeared.
* `QQuickTextDocument` access from a `TextEdit`, and `toHtml` stability across saves (S1).
* `QSaveFile` temp-file names and how Nextcloud treats them.
* `KDBusService` Unique mode: how the name is built, and whether Wayland activation tokens need `KWindowSystem`.
* `QML_ELEMENT` and `QML_SINGLETON` from a non-QML core library.
* `QTextDocument::toMarkdown` support for underline, strikethrough, images and bullets (only matters if S1 picks Markdown).
* Whether `qt6-wayland` is needed at runtime.
