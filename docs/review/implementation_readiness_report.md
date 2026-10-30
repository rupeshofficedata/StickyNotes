# Implementation Readiness Report

**Target:** `docs/designs/sticky_notes.md` (lines 316-374, Detailed Implementation Plan; lines 1-315 read as context)
**Mode:** REVIEW. **Reviewers:** egm-implementation-readiness, egm-codebase-details (standard: implementation-readiness-standard.md), egm-style (persona defaults).
**Scope note:** Greenfield project. Qt, KF6, KDBusService, KStatusNotifierItem and QSaveFile claims were checked only against the document itself. Items needing outside verification are listed at the end. Earlier reports were not used.

**Verdict: FAIL against the Zero-Decision Rule.** Blockers: 0. Major: 10. Nit: 15.

## Prioritized Findings

### M1. Qt floor and note format are called "confirmed" but are provisional - Severity: Major
* **Lines:** 318, 324 (context: 54-56, 150, 246-250)
* **Description:** Line 318 says "Qt minimum 6.5 (confirmed by S1 and S3)". Open Questions (54-56) say the floor is provisional and the format depends on S1. The Spikes table has no results. S3 is a Flatpak spike and Flathub is deferred, so it cannot confirm the floor for the AUR build. Files and tests at 334, 348 and 351 assume `note.html`; no step says what changes if S1 picks Markdown.
* **Rationale:** The implementer must pick the format and Qt version after S1, which violates the zero-decision rule. Every Stage 1 file depends on it.
* **Fix:** Either record the S1/S3 results in the Spikes section and close the Open Questions, or change line 318 to "Qt minimum 6.5 (provisional until S1 passes)" and add under Stage order: "Stage 1 starts only after S1 is recorded; the plan is then edited to the final format." State which spike confirms the floor for AUR (S1) and that spike code lives in `spikes/` and is not in the file list.

### M2. No file-to-stage mapping or per-stage exit checks - Severity: Major
* **Lines:** 322-328, 330-361
* **Description:** The 29 files and the 5 tests are not assigned to stages. `main.cpp`, `CMakeLists.txt`, `NoteWindow.qml` and `PrimaryWindow.qml` are touched by several stages as single Create items. `Theme.qml` and `ErrorBanner.qml` are needed by the Stage 1 note window, but colors and banners are in Stage 3. `BinView.qml` is Stage 1, `SettingsPage.qml` Stage 3. Paste/drop is built in Stage 1 (325) while "images" is Stage 3 (327), so `RichTextBridge` image code sits in both. `tst_applifecycle` and `single_instance_test.sh` need Stage 2. S6 is "built here" (325) but "not a gate, runs while building Stage 1" (250).
* **Rationale:** Stages must leave a buildable, testable tree. Without a map the implementer decides the order.
* **Fix:** Tag every file with its stage and list per-stage deltas for the multi-stage files. Add one exit check per stage, for example "Stage 1: build passes, `tst_notestore` and `tst_noteindex` pass". Put paste/drop in one stage only and reword Stage 3 "images" to the 10 MB limit, placeholder and link rules. Move `Theme.qml` and `ErrorBanner.qml` to Stage 1 (or give Stage 1 a stub).

### M3. Public C++/QML APIs are named but not specified - Severity: Major
* **Lines:** 332-361 (context 219-225)
* **Description:** Only four `NoteStore` signals and four `AppLifecycle` methods are given. Missing: `NoteStore` methods (create, load, save, delete, restore, emptyBin, deleteForever, setFolder), `NoteIndex` roles and filter property, whether the Bin list is a second model or a mode, `RichTextBridge` properties and invokables, `PlatformCaps` property names, how `NoteWindow` gets body and color token, who owns the 400 ms save timer, the QML import URI and version.
* **Rationale:** These are design decisions left to the implementer, and tests cannot be written without them.
* **Fix:** Add a header-level table per class (name, type, signature, thread, error return). Name the registration mechanism (`QML_SINGLETON`/`qmlRegisterSingletonInstance`) and the import URI, for example `io.github.rupeshkumar.StickyNotes 1.0`. Decide the save-timer owner.

### M4. Signal handling, save flush and quit-prompt mechanics are unspecified - Severity: Major
* **Lines:** 333, 336 (context 186, 189)
* **Description:** "Connects SIGTERM and SIGINT to `quit()`" gives no mechanism or file (the `QSocketNotifier` helper has no home other than `main.cpp`). "Flushes saves before quitting" has no dirty-note registry, flush call or ordering against `aboutToQuit`. The Retry / Discard / Cancel prompt (189) has no owner in `AppLifecycle` or QML.
* **Rationale:** This is the data-loss path the design cares most about, and it is left to guesswork.
* **Fix:** Define `NoteStore::flushAll()` (synchronous, returns failed ids). `AppLifecycle::quit()` calls it; on failure it emits `quitBlocked(ids)`, handled by a named QML dialog. Name the signal mechanism (socketpair plus `QSocketNotifier`) and say it is a small class in `main.cpp` or give it a file. SIGTERM skips the prompt per line 189.

### M5. Rescan and external-change algorithm is not specified - Severity: Major
* **Lines:** 334, 335 (context 197-206)
* **Description:** "Newer" is not defined (mtime compared to what, equality or greater-than, size). Own writes cannot be told from outside edits without a watcher, and the mtime recorded after the `QSaveFile` commit is not stated. The trigger (`focusWindowChanged` vs `applicationStateChanged`), debouncing, thread, and the partial-write retry rule are missing. No component owns the open-note registry that emits `noteChangedOutside` and `noteRemovedOutside`.
* **Rationale:** Getting this wrong either loses outside edits or reloads over the user's own save.
* **Fix:** Add pseudocode for `rescan()`: record mtime and size at load and after each save, compare on rescan, emit per open note. State the trigger signal, that it runs on the GUI thread, and that `NoteStore` owns the open-note registry.

### M6. Build configuration and component layering are unresolved - Severity: Major
* **Lines:** 332, 336, 339 (context 133-137, 221-222)
* **Description:** `CMakeLists.txt` gives no minimum CMake version, `find_package` components, target names, `qt_add_qml_module` URI, QML resource handling, or install destinations. `AppLifecycle` lives in the core library (no Qt Quick) yet is a QML singleton that shows and hides windows. The registration location and how core learns window visibility (QML calls in, or `main.cpp` passes a callback) are not stated. Also unstated: whether `ECM` is required.
* **Rationale:** Core cannot link Qt Quick by the document's own diagram, so the lifecycle boundary is a real design gap.
* **Fix:** Add a CMake bullet list: `cmake_minimum_required`, `find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui DBus Quick QuickControls2 Test)`, `find_package(KF6 REQUIRED COMPONENTS DBusAddons StatusNotifierItem)`, ECM if needed, target names, install dirs. State that singletons are registered in the app target and that `AppLifecycle` receives window visibility through an interface or calls from QML.

### M7. PKGBUILD step leaves package names for the implementer to confirm - Severity: Major
* **Lines:** 357
* **Description:** "Confirm the names against the Arch package database" is an open instruction inside a plan. `makedepends` (cmake, extra-cmake-modules), `source`, `sha256sums`, `pkgver`, `license` and the build/package functions are absent. No `makepkg`/`namcap` check in the verification plan.
* **Rationale:** An unresolved lookup and an incomplete package definition break the zero-decision rule for the AUR release.
* **Fix:** Verify the names now and write them in, or label the list "unverified" and add a pre-step to Stage 4. List all PKGBUILD fields and add `makepkg` plus `namcap` to Verification.

### M8. Tray detection, tray item, D-Bus files and clash rule have no behavior spec - Severity: Major
* **Lines:** 333, 337, 348, 353-354 (context 172, 178-179, 235)
* **Description:** `PlatformCaps` "tray present" has no stated check (for example a StatusNotifierWatcher lookup). Nothing says who builds the `KStatusNotifierItem`, its menu or click actions. The `.service` `Exec=` and `.desktop` keys are not given. "Restore refused on ID clash" is tested (348) but defined nowhere. "Delete forever" and `meta.json` parse failure are undefined beyond labels.
* **Rationale:** The test and the lifecycle table assert behavior the document never defines.
* **Fix:** Add a short spec for each: tray probe API, tray menu entries and actions, service and desktop keys, and a restore-clash rule in "Note on disk" or the delete section.

### M9. Test plan lists topics, not cases, and misses design edge cases - Severity: Major
* **Lines:** 347-352, 367
* **Description:** No fixtures or assertions (for example how a failed save is provoked, expected signal, retry timing). `tst_applifecycle` "every row" needs a fake tray and window set, but no seam is defined for a core class. `tst_richtextbridge` needs a harness. Missing tests: 10 MB limit and non-image drop, 400 ms debounce, 10 s retry, failed-save quit prompt, SIGTERM flush, all four external-change cells, title rule (60 chars, "Untitled"), cascade wrap after 8, `QSettings` removal on forever-delete, error row. Item 21 does not say what "shows the first" is checked with, and a Wayland check needs a compositor. Line 367 promises the data-loss paths, but the retry and quit prompt are not in the list.
* **Rationale:** Tests cannot be written to a topic list, and the stated coverage bar is not met by the listed tests.
* **Fix:** Add a case table per test file (setup, action, expected result). Define test seams such as constructor-injected tray and window probes, and run with `--platform offscreen`. Specify the single-instance check, for example a D-Bus `NameHasOwner` call or log line.

### M10. Verification commands lack flags and acceptance criteria - Severity: Major
* **Lines:** 364-366
* **Description:** `clang-tidy` has no config and no `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`. `qmllint src/qml/*.qml` needs import paths and will report unresolved singleton types. No pass criterion is given. The build command has no `CMAKE_PREFIX_PATH`/ECM note.
* **Rationale:** An implementer cannot tell a pass from a fail.
* **Fix:** Give exact commands (`qmllint -I build/<qml-module-dir> ...`, `clang-tidy -p build`) and an acceptance rule (zero warnings, or a named allowlist). Mark qmllint non-gating if import resolution is not possible.

### N1. QML files have no UI spec; GNOME fallback header has no trigger - Severity: Nit
* **Lines:** 341-343 (context 236)
* **Description:** Toolbar button list and order, banner texts, list-button glyph, and where `NoteWindowManager` is instantiated (it must live while the primary window is hidden) are unstated. `PlatformCaps` reports only tray and Wayland/X11 (235), so detecting "no server-side decorations" is undefined.
* **Fix:** Add a short UI spec per QML file, give an SSD detection rule, or drop the GNOME fallback header from v1.

### N2. BinView confirmation and forever-delete ordering unspecified - Severity: Nit
* **Lines:** 345 (context 212)
* **Fix:** State the confirmation (dialog text, default button) and the order: delete the folder, then remove the `QSettings` entry.

### N3. README items are not staged or structured - Severity: Nit
* **Lines:** 358-359
* **Description:** `src/README.md` is "after the code exists" but has no stage. The README item (358) is one long list of unequal parts.
* **Fix:** Assign `src/README.md` to Stage 4. Shorten item 27 to: "Install, notes location, folder change, synced folders, desktop support matrix, KDE always-on-top rule, bug-report checklist".

### N4. LICENSE text source - Severity: Nit
* **Lines:** 359
* **Fix:** Say to copy the verbatim GPL-3.0 text from gnu.org.

### N5. `main.cpp` application identity and logging category names - Severity: Nit
* **Lines:** 333, 338 (context 240)
* **Description:** Not stated: the engine load URL, `setQuitOnLastWindowClosed(false)`, `setOrganizationName`/`setApplicationName` (which `QSettings` depends on), and the exact category strings.
* **Fix:** List them, for example `stickynotes.storage`, `stickynotes.tray`, `stickynotes.activation`.

### N6. PKGBUILD omits optdepends - Severity: Nit
* **Lines:** 357
* **Fix:** Add `optdepends` if any, and the `makepkg`/`namcap` check (see M7).

### N7. Packaging file keys and install paths missing - Severity: Nit
* **Lines:** 353-356
* **Fix:** List required `.desktop` keys (`Exec`, `Icon`, `Categories`, `StartupWMClass`), `.service` keys (`Name`, `Exec`) and destinations (`share/applications`, `share/dbus-1/services`, `share/icons/hicolor/scalable/apps`, `share/metainfo`).

### N8. C++17 stated without a compiler floor - Severity: Nit
* **Lines:** 318
* **Fix:** Name minimum GCC/Clang versions or drop the point.

### N9. SettingsPage lists a non-setting - Severity: Nit
* **Lines:** 344 (context 312)
* **Description:** "Color scheme follows system" appears as a settings item, but an override is a Future Improvement.
* **Fix:** Remove it, or say it is a read-only label.

### N10. `ErrorBanner.qml` action API - Severity: Nit
* **Lines:** 346 (context 94, 188-189)
* **Description:** Banners have different button sets (Reload / Keep mine, Retry).
* **Fix:** State that it takes a message and a list of action labels and signals back to the caller.

### N11. "(Create)" tag is redundant - Severity: Nit
* **Lines:** 330, 332-360
* **Fix:** Rename the heading "Files to Create", say "All files are new" once, drop per-item tags.

### N12. Undefined labels and missing cross-references - Severity: Nit
* **Lines:** 318, 325, 336
* **Description:** S1, S3 and S6 are used without a pointer to the Spikes section. "The lifecycle table" has no section reference. "StickyNotes" is bold on line 318 while the App ID and license are not.
* **Fix:** Add "(see Spikes)" and "(see Lifecycle)" references. Drop the bold. Use "StatusNotifierItem" (KF6 component) and "KStatusNotifierItem" (class) consistently.

### N13. Test and verification wording - Severity: Nit
* **Lines:** 348, 351, 367, 368
* **Description:** "ID clash" is informal. "Round-trip" is jargon. 367 is an intent statement that duplicates 348-351. 368 uses fragments and repeats item 21.
* **Fix:** Use "ID conflict". Write "Format toggles survive save and reload". Replace 367 with "each listed data-loss path has at least one test". Rewrite 368 as "Manual checks: ..." and refer to `tests/single_instance_test.sh`.

### N14. Duplicate provenance and Stage 4 framing - Severity: Nit
* **Lines:** 328, 372, 374
* **Fix:** Keep one of the provenance comment and the footer. Optionally prefix Stage 4 items with "packaging:".

### N15. Plasma 6.4 floor has no stated reason - Severity: Nit
* **Lines:** 13, 62 (outside range)
* **Fix:** State which feature needs 6.4, or relax the floor.

## Claims needing outside verification (not checked)
* `Qt.styleHints.colorScheme` and the QtCore `Settings` QML type (versus `Qt.labs.settings`) being available at Qt 6.5 (lines 55, 225). S1 is meant to cover this.
* `KDBusService` with `DBusActivatable=true` cooperating with the service file, and a second launch raising the first via `activateRequested` (Stage 2, `single_instance_test.sh`).
* How tray presence is detected (a StatusNotifier host check is not a given).
* `QSaveFile` rename behavior on synced folders and across filesystems (185-187).
* `startSystemMove()` and `startSystemResize()` on frameless QML windows under KWin (236).
* KF6 `find_package` component names and ECM requirement.
* Arch package names `qt6-base`, `qt6-declarative`, `kdbusaddons`, `kstatusnotifieritem` (357).

## Checks that passed
App ID, name and license are consistent across 318 and the packaging names. The lifecycle table matches the `AppLifecycle` description and CUJs 3 and 4. The core/app split in the diagram matches the CMake description and `RichTextBridge` placement. The color table agrees with `Theme.qml`.

## Severity disagreement between runs

A second, direct run of the implementation-readiness reviewer rated the same plan **FAIL with 5 Blockers, 7 Majors and 5 Nits**. The findings overlap with M1 to M10 above. The differences are in severity, not in substance:

* Blocker there, Major here: Stage 0 and the "confirmed" Qt claim (M1); missing signatures and call flows for the core classes (M3, M4); topic-only test list (M9); undefined behaviors such as restore on ID conflict, how "changed outside" is detected, and a crash between writing `meta.json` and `note.html` (M5, M8); one-sentence `CMakeLists.txt` and unverified AUR names (M6, M7).
* Extra points in that run: `meta.json` has no atomic-write or invalid-token rule; the "removed outside" case does not say whether `meta.json` and `images/` are recreated; `SettingsPage` does not define the folder dialog or what happens to open notes when the folder switches; `BinView` does not define behavior when `.bin` already holds an ID.

The style reviewer found no Blockers.
