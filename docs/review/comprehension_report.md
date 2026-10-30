# Comprehension Report (Stage 6)

* **Target:** `docs/designs/sticky_notes.md`. FeatureRange: lines 1-306. DetailedRange: lines 307-519.
* **Standard:** `re-implementation-standard.md`
* **Reviewer:** egm-comprehension (naive reader, document only)

**Verdict: FAIL.** The intent is clear. An engineer could not write identical code from this document without guessing. The note file format is unresolved, many numeric contracts are missing, and several sections contradict each other.

## Blockers

* **B1. Note file format is undecided** (lines 57, 153, 358, 459, 465). `<ext>` appears in the disk layout, `NoteStore` and the tests. Serialization, the `RichTextBridge` round trip and the byte-identical second save cannot be written. *Fix:* state a provisional default (for example HTML via `QTextDocument::toHtml`, since Markdown cannot store underline) with the extension, or mark the document blocked on S1 and list which contracts change under each outcome.
* **B2. No action rules for spike results** (lines 56, 311, 315, 485). The document does not say what to build if a spike fails: S3 failing, S2 finding no Qt session API (line 418 says "the Qt session API found in S2"), or S4 missing the 200 ms target. *Fix:* give each spike a pass/fail rule and a default design on failure. Name the Qt API for session restore, or remove it from Stage 4.
* **B3. Watch set contradicts itself** (lines 28, 192, 194, 368). Line 192 says "a watcher on the notes folder". Line 194 says one watch per note folder. If 900 of 1,000 watches succeed, it is unclear whether the fallback is partial or total, and how often the notice appears. *Fix:* state the exact watch set (notes folder, each note folder, `.bin`?), whether a partial failure disables the watcher, and the notice text and frequency.
* **B4. Lifecycle table versus goals and CUJs** (lines 36, 79, 85, 159-166). "Last window" means the last visible window of any kind (line 159), which conflicts with "close last note, primary visible: stay alive". In the no-tray column, closing the primary window with no open note quits immediately, so hide-to-background with zero notes cannot be reached. *Fix:* replace the table with a complete one over mode x tray presence x primary visible or hidden x note count, and define "last window" per row.
* **B5. Hide mode when no tray exists** (lines 36, 79, 164, 408). It is not stated whether the stored setting is overwritten or overridden at runtime, when tray detection runs (startup only, or watched), or the notice text.
* **B6. Note ID format** (lines 116, 181, 353, 373). `QUuid::toString()` returns braces and the ID becomes a folder name. *Fix:* UUID v4, lowercase, dashes, no braces, via `QUuid::toString(QUuid::WithoutBraces)`.
* **B7. Image storage contract** (lines 32, 155, 206, 358, 393). Missing: file naming in `images/`, image format, size cap, downscale threshold, link syntax inside the chosen format, `suggestedName` for clipboard data, behavior for non-image drops, placeholder appearance. *Fix:* give concrete values (for example 4096 px longest edge, PNG, `<uuid>.png`, cap in MB).
* **B8. Orphaned images** (lines 155, 358). No rule for files in `images/` that the note no longer references, or for deleting an image inside the editor.
* **B9. Bin retention wording** (lines 34, 271, 443). Alternative E mentions "a retention rule" while BinView says no automatic removal. *Fix:* delete the phrase from line 271.
* **B10. Bin and restore name clashes** (lines 34, 103, 358). Policy when a note with the same ID exists on restore, or when `.bin` already holds that ID, is not stated.
* **B11. Move refusal rules differ** (lines 27, 91, 181-186, 373). CUJ 5 says open notes are closed, line 27 says "protects", line 373 says only "flush saves". The refusal condition is worded three ways. Not stated: whether the target must be empty, whether the target may be inside the source, whether notes reopen afterward.
* **B12. Move verification and partial delete** (lines 183-186, 373). "Verify (file list and sizes)" is vague. A failure halfway through deleting originals breaks "remove nothing from the old folder" (line 186), and the resulting state is not described. Not stated whether `.bad` folders move.
* **B13. "Keep mine" and "Reload"** (lines 28, 97, 195, 433). Not stated: how "newer" is compared, whether the banner returns on a second change, what Reload does with unsaved edits, how an open note with no unsaved edits behaves, and what happens when an open note is deleted externally. *Fix:* a state table of open/closed x dirty/clean x changed/removed/added.

## Major

* **M1. Numbers and timings.** Save debounce "about 400 ms" versus watcher "about 300 ms". No retry interval for failed saves. Search ceiling 10,000 notes versus the 1,000-note S4 test. The 200 ms rescan target appears only in spike criteria.
* **M2. Title and snippet derivation, list sort order** are missing.
* **M3. Metadata schema incomplete.** Schema version value, date format, exact keys, allowed color tokens, behavior on a newer schema or a corrupt `meta.json`, and whether a bad `meta.json` triggers the unreadable-folder rule. *Fix:* a JSON example and a failure matrix.
* **M4. `.bad` handling.** What if `<id>.bad` exists, where the notice appears and for how long, and how the index and watcher tell `.bad` and `.bin` apart from note IDs.
* **M5. Geometry store contract.** No JSON schema, units, screen identification, default size or minimum size. "Periodic cleanup at startup" is contradictory. Whether primary window geometry is saved is not stated.
* **M6. Cascade rule.** Offset distance, wrap-around, first position and position after restart.
* **M7. `WindowPlacer` and `PlatformCaps` overlap.** Who decides "honors positions", and how. The capability matrix and degradation table are referenced but not written.
* **M8. Color feature.** No hex values for the six tokens in light and dark, no statement whether color applies per note or per selection, and the fixed-palette UI is not described.
* **M9. Color scheme override.** How a change applies live.
* **M10. Qt version stated three ways** (lines 59, 309, 343). Confirm QML exposure of `Qt.styleHints.colorScheme` in 6.5.
* **M11. Single instance.** `DBusActivatable=true` needs a D-Bus service file, which is not in the file list. `KDBusService` options and Wayland focus behavior are not specified.
* **M12. Flatpak manifest incomplete.** App ID, runtime, SDK, command, `--filesystem` rules, how `$XDG_DATA_HOME` behaves in the sandbox, and a conflict between "portal only" and the default notes folder.
* **M13. CMake and layering.** The "core library that does not link Qt Quick" contains `RichTextBridge`, which uses `QQuickTextDocument`. `QApplication` (Widgets) is used without explanation.
* **M14. Component names differ** between the architecture diagram and the file plan (`SettingsStore` versus `AppSettings`; `FolderMover`, `PlatformCaps`, `WindowPlacer` missing from `main.cpp` or the diagram).
* **M15. C++/QML interface.** Registration mechanism, properties, signals and invokables, how each note window gets its own `RichTextBridge`, who creates note windows, and the `TextEdit` `textFormat`.
* **M16. Delete UI.** Confirmation, what happens to the window, list row layout, per-note actions, click or double-click to open.
* **M17. Search semantics.** Substring or token match, locale handling, whether it applies to the bin.
* **M18. Save-failure interplay.** Dialog wording and options, and SIGTERM with a failed save.
* **M19. Signal handling.** A POSIX signal handler cannot call Qt directly. The mechanism (`QSocketNotifier` and a socket pair) is not specified.
* **M20. Tray details.** Icon name, tooltip, left-click behavior, and the display-name inconsistency ("StickyNotes" versus "Sticky Notes for Linux").
* **M21. Failed save at last-note close in CUJ 4.** Prompt before the note closes, or at quit.
* **M22. Primary window UI.** Tabs, notice locations, hide controls, initial size, startup visibility.
* **M23. First-run and unavailable-store behavior.** Which controls work, recovery path, failure to create the default folder, how to pick another folder.
* **M24. Test plan traceability.** `RichTextBridge` needs GUI tests. `PlatformCaps`, QML and single-instance tests have no files. The 80% target does not match the core-only link target.
* **M25. EGM contracts.** "EGM interface contracts" and `egm-bootstrap` are not defined for a reader without the plugin.

## Nits

* N1. Undefined terms: EGM, CUJ, SNI and StatusNotifierWatcher, `xx-zones-v1`, `xdg-toplevel-groups`, "Bing-style insights".
* N2. "Survive uninstalls" conflicts with Flatpak uninstall that deletes user data.
* N3. "Documented" without saying where. The README table contents are unspecified.
* N4. Logging section gives no levels, file location or debug switch (suggest `QT_LOGGING_RULES`).
* N5. Stage numbering is unrelated to this document.
* N6. Line 240 refers to a conversation the reader never saw ("the failure condition that was presented").
* N7. Alternative D lists the chosen option, not the rejection reason for each listed option.
* N8. KF6 component names differ across sections (`StatusNotifierItem`, `KStatusNotifierItem`, `kstatusnotifieritem`).
* N9. `src/CMakeLists.txt` and the QML module setup are not in the file list.
* N10. Always-on-top is out of scope, but the README promises KDE rule steps that are not specified.

## Decisions the reader would have to make alone

1. Note file format and extension. 2. UUID string form. 3. Image naming, format, cap, downscale and link syntax. 4. Title and snippet derivation, list sort order. 5. Color token values and picker UI. 6. Default and minimum note size, cascade offset and wrap. 7. Geometry JSON schema. 8. Debounce values and retry cadence. 9. Dialog text for save-failed quit, move confirmation and notices. 10. QML registration and invokable API. 11. Signal-safe SIGTERM mechanism. 12. D-Bus service file. 13. Flatpak manifest content. 14. Watch set and partial-failure handling. 15. Fallback when spikes fail. 16. External-change state table. 17. Complete lifecycle table. 18. Partial-delete recovery in Move.
