# Intermediate Zoom Out Report (Stage 8)

* **Target:** `docs/designs/sticky_notes.md` (full file, 519 lines, unedited since Stage 5)
* **Standard:** `zoom-out-standard.md`
* **Inputs:** `zoom_out_report.md` (Stage 5), `comprehension_report.md` (Stage 6), `critic_report.md` (Stage 7)

**Verdict:** Context is sufficient. The design is still over-scoped for a first release. Stages 6 and 7 mostly ask for more detail on the same features that Stage 5 said to cut. Cut or defer those features first, then write detail only for what remains.

## 1. Stage 5 findings still open

All 11 Stage 5 findings are open because the document has not been edited. Stage 7 B1 independently flags folder Move as the only Blocker. Stage 5 treated these as owner choices that stand until the owner decides otherwise. No such decision is recorded in the document.

## 2. Do Stages 6 and 7 push toward more complexity?

Yes, in many places. Taken literally, the fixes would add:

* **For Move (Stage 7 B1, Stage 6 B11, B12):** content hashes, a journal or commit marker, resumable deletes, `.moved` renames with deferred deletion, nesting and non-empty-target rules. That is a transaction engine for a feature Stage 5 says to cut.
* **For external changes (Stage 7 M2, M3, Stage 6 B3, B13):** a revision field, settle delays, conflict-copy handling, a full open/closed x dirty/clean x changed/removed state table, and a 10,000-note watcher test.
* **For lifecycle (Stage 7 M7, Stage 6 B4, B5):** a bigger table and live tray watching, against Stage 5's request for fewer rows.
* **For geometry (Stage 6 M5 to M7):** a JSON schema, cascade numbers and a `WindowPlacer` contract for components Stage 5 says to delete.
* **Other:** a recovery copy in the state directory (Stage 7 M8), a background text index (M12), a non-KDE single-instance and tray path (M13), and pass/fail rules for six spikes (Stage 6 B2).

A large share of Stage 6 and 7 findings disappear if the Stage 5 cuts are made first.

**Findings that hold even after simplifying:**
* Pick a provisional note file format and extension (Stage 6 B1). HTML is the default, since Markdown cannot store underline. S1 can change it.
* Fix the note ID form: lowercase UUID, no braces (B6).
* Define the image contract: file naming, format, size cap, orphan rule (B7, B8). Confine image paths to the note's `images/` folder and block remote image loads (Stage 7 M9). This is a real security boundary on a folder that sync tools write to.
* Delete the "retention rule" phrase at line 271 and state the restore ID-collision rule (B9, B10).
* Fix the layering: `RichTextBridge` uses `QQuickTextDocument` but sits in a core library that "does not link Qt Quick" (M13).
* Use Qt's own quit path and `aboutToQuit` instead of raw signal handlers (M19).
* `DBusActivatable=true` needs a D-Bus service file that is not in the file list. Add it or drop the flag (M11).
* State the loss window honestly: the 400 ms debounce and no directory fsync (Stage 7 M8).
* Add the plaintext-in-synced-folder trade-off as an accepted risk (Stage 7 M11).
* "Survive uninstalls" is false for Flatpak. Fix the wording or drop Flatpak from v1 (Stage 6 N2).
* Wording fixes (Stage 7 M15 to M18 and the style nits). They reduce text.

## 3. Reconciled view

**Simplify first (decide, record in the document, then drop the matching detail):**
* Folder setting means "use this folder". No Move, no `FolderMover`, no CUJ 5 delete branch. Add an "Open notes folder" button.
* External changes by rescan on focus plus Refresh, with the banner driven by modification time. No watcher, no S4, no setting. A small state table for open notes only.
* One lifecycle rule derived from tray presence, no hide-mode setting. At most three rows. Show the primary window if no tray is present at startup. No live tray watching.
* `QSettings` for window size, and position on X11 only. No `GeometryStore`, `WindowPlacer`, `geometry.json` or session management.
* Three spikes: S1 (format), S3 (Flatpak), S6 (paste, if suspect). Results go in the design, each with a one-line fallback.
* Drop the `meta.json` schema version and unknown-field retention. Keep folder per note.
* Cut extra colors, the color-scheme override, image downscale and the 10,000-note ceiling. Show unreadable notes as an error row and leave them in place.
* Call `KDBusService` and `KStatusNotifierItem` directly with no wrapper classes.
* Reduce the plan to one-line responsibilities and keep only cross-boundary contracts.

**Add detail (real gaps):** file format default and extension, ID form, image contract and path confinement; the C++/QML interface; the `NoteStore` failure contract and the exact save-failure UX at quit and last-note close; the short lifecycle table with quit prompt options (save, discard, cancel); search semantics and sort order in two lines; a short Risks section; one Qt floor value.

## Ranked findings

**Blocker**
1. No recorded scope decision on the Stage 5 cuts (lines 27, 28, 29, 36, 56). All 11 Stage 5 findings are open and Stages 6 and 7 add rules on top of features Stage 5 says to remove. *Fix:* the owner decides now on Move, the watcher, the geometry store and the hide-mode setting. Record it in the Decided design choices table and strike the matching goals, CUJ 5, components and files.
2. Folder Move is a data-loss hazard and the largest block of added detail requested by Stage 7 B1 (lines 27, 87-91, 179-188, 370-374). *Fix:* remove Move for v1. If kept, copy only and never delete the originals.

**Major**
3. Adopting Stage 7 M2 and Stage 6 B13 literally would grow external-change handling into a state table, hashes and conflict-copy handling. *Fix:* rescan on focus plus Refresh, modification-time comparison, a banner for an open note with unsaved edits only, one sentence on Reload and Keep mine.
4. Note file format unresolved (lines 57, 153, 358, 459). *Fix:* HTML with `.html` as the provisional default, and list what changes if S1 picks Markdown.
5. Spike stage too large (lines 56, 225-232, 329-338). *Fix:* S1, S3, optionally S6, in one table of decision, pass criterion and fallback.
6. Lifecycle table over-specified and ambiguous (lines 158-171). *Fix:* three rows derived from tray presence, with "visible" and "last window" defined once.
7. Image and note-link security and contract missing (lines 32, 155, 206, 358). *Fix:* allow PNG, a naming rule, one size cap, relative paths confined to `images/`, no remote loads, and a rule to leave orphans.
8. Detailed plan sits in the weeds (lines 343-348, 368, 378, 393, 455-465). *Fix:* one-line responsibilities, one debounce value.
9. Core library layering (lines 135, 343, 390). *Fix:* move `RichTextBridge` out of the core target, or state the exact modules.
10. Platform and geometry components are speculative wrappers (lines 136-140, 375-378, 400-418). *Fix:* delete `GeometryStore`, `WindowPlacer`, `SingleInstance`, `TrayIcon`; keep one `PlatformCaps`.
11. Flathub in release 1 creates portal, sandbox and tray conflicts (lines 13, 40, 236, 482-485). *Fix:* AUR first, Flathub after S3 passes.
12. Save-loss window and failed save at quit not honestly stated (lines 171-177, 348). *Fix:* state the loss window; use `aboutToQuit`; no recovery copy in v1.
13. C++/QML interface undefined (lines 109, 129-135, 348, 393, 433). *Fix:* a short contract of objects exposed to QML and who creates each note window's `RichTextBridge`.
14. Missing Risks section. *Fix:* a short table of accepted risks.
15. Alternatives are inconsistent and carry meta-commentary (lines 240, 252, 258, 260-295). *Fix:* delete line 240, keep A to C in full, fold D to I into one-line rationales.

**Nit**
16. Qt floor appears three times (lines 59, 309, 343). Pick 6.5 provisional.
17. Note ID form (line 353).
18. Line 271 mentions a retention rule that does not exist.
19. `DBusActivatable=true` needs a service file (lines 121, 470).
20. Four directory READMEs, and "EGM" is undefined for a reader without the plugin (line 502).
21. 80% coverage target (line 512).
22. Line 44 "Bing-style insights", duplicate spike list at line 56, duplicate "Markdown cannot store underline" at lines 57 and 153, and the Future Improvements heading level at line 299.
23. Component names differ between diagram and file plan (lines 134 and 380).

## Sequencing advice

Make the scope decisions in Blocker 1 first. Then edit the document to remove the cut features. Then close only the Stage 6 and 7 gaps that remain. The other order would write detailed rules for features that are then deleted.
