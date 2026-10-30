# Critic Review Report

**Target:** `docs/designs/sticky_notes.md` (lines 1-306; Detailed Implementation Plan from 307 excluded)
**Mode:** REVIEW. **Reviewers:** egm-design, egm-structural, egm-style. Greenfield project.
**Standard:** feature-spec-standard.md (design, structural); style used persona defaults.
**Totals:** 1 Blocker, 18 Major, 26 Nit (after deduplication).

Lens tags: D = design, S = structural, T = style.

## Blocker

### B1. Folder-change (Move) procedure can lose data. Lines 27, 91, 179-188 [D]
* **Description:** Verification checks file list and sizes only. There is no crash-recovery marker between "save new path" and "delete originals". Unspecified cases: target nested inside the source or the reverse, a non-empty target, and cross-device moves. The name-clash check (step 1) runs before the watcher is paused (step 2), so a sync client can add a clashing name in between. Line 27 says open notes are "protected" but step 2 closes them, and nothing covers unsaved edits or a note that fails to close.
* **Rationale:** This is the one path that deletes user data in bulk (40+ folders in CUJ 5). Recommend content hashes or an mtime plus size re-stat, a journal or commit marker so the delete is resumable and idempotent, a re-check after pausing the watcher, explicit rules for the nesting and non-empty cases, and renaming originals to `.moved` with deferred deletion instead of immediate deletion.

## Major

### M1. No Risks & Mitigations section. Whole doc (nearest: lines 54, 238) [D, S]
* **Description:** The standard requires one. Risks are scattered (lines 29, 40, 156, 171-177, 194, 199, 210, 265, 271) and security implications appear nowhere.
* **Rationale:** A reader cannot find failure modes, security and scalability limits in one place. Add a table of risk, impact and mitigation, seeded from the findings in this report.

### M2. External-change conflict model under-specified. Lines 28, 97, 156, 190-195 [D]
* **Description:** "Newer" is judged by mtime, which sync clients and clock skew make unreliable. Not covered: an open note with no unsaved edits, external deletion or rename of an open note, sync conflict copies and stray folders, and a half-synced note (`meta.json` without `note.<ext>`). The `.bad` move-aside could quarantine a note that is only mid-sync, and renaming inside a synced tree causes new sync conflicts. The own-write ignore mechanism (hash, token or time window) is not stated. Line 28 says "never overwrite" but line 195 lets Keep mine overwrite.
* **Rationale:** Use a content hash or revision in `meta.json`, add a settle delay and retry before declaring a note bad, define the missing cases, and reword line 28 to match line 195.

### M3. Watcher scale does not match the stated ceiling. Lines 26, 194, 210, 281 [D]
* **Description:** S4 measures 1,000 notes but the stated ceiling is 10,000. The fallback (rescan on focus) leaves open notes stale, which line 281 names as a weakness. A 10,000-folder rescan on FUSE or network mounts may be slow.
* **Rationale:** Reconcile the two numbers, set S4 pass/fail at the ceiling, and bound the rescan (top-level stat plus a per-note mtime cache).

### M4. Note file format undecided yet load-bearing. Lines 25, 57, 150-156, 227 [D]
* **Description:** Search, external-change comparison and the "formatting survives unchanged" goal all depend on the format. Qt HTML export is verbose and not guaranteed byte-stable across Qt versions, so a Qt upgrade could re-serialise every note and trigger a sync storm. There are no S1 acceptance criteria beyond "second save byte-identical" and no fallback if neither HTML nor Markdown passes.
* **Rationale:** State S1 pass criteria, the fallback, a format version in `meta.json`, and whether lock-in to Qt's HTML dialect is accepted.

### M5. Flatpak and portal behavior conflicts with the goals. Lines 28, 40, 229, 236, 281 [D]
* **Description:** Portal (FUSE) paths make inotify unreliable and change the real path. `$XDG_DATA_HOME` inside Flatpak is private, so the default folder is not where the user expects, and AUR and Flatpak builds do not share data. Flathub is a goal but is gated by unfinished spike S3.
* **Rationale:** Decide whether Flathub is a hard requirement or a stretch, what is dropped if S3 fails, and how the default path behaves across builds.

### M6. Wayland position outcome has no defined user-visible behavior. Lines 22, 29, 49, 197-201 [D]
* **Description:** Position restore is a non-goal, yet geometry storage is a goal. Not stated: which mechanism is used on Plasma, what the user sees on GNOME, whether size restore works where position does not, who writes `geometry.json` and when, and how the app knows the compositor ignored a position.
* **Rationale:** Without this, `geometry.json` may store data that is never applied. Specify the outcome per desktop.

### M7. Lifecycle gaps. Lines 36, 122, 160-169 [D]
* **Description:** "Visible window" is undefined for minimised windows, and Wayland does not report minimise reliably. Tray presence is "detected at runtime" but not re-evaluated, so a tray that disappears (panel restart, extension toggled) leaves a hidden primary window unreachable. A background-mode user with all windows hidden has no surface and can return only via the launcher.
* **Rationale:** Define "visible", watch the StatusNotifierWatcher and show the primary window if the tray vanishes, and state the background-mode return path.

### M8. Save failure at quit, SIGTERM and logout. Lines 171, 175-177 [D]
* **Description:** "Asks before quitting" cannot work on SIGTERM or shutdown. The 400 ms debounce loss window on SIGKILL or power loss is unstated, as is fsync behavior (QSaveFile does not fsync the directory).
* **Rationale:** Write a recovery copy to the state dir, offer recovery at next launch, and state the accepted loss window. Otherwise the "never silent, survives crashes" goal has a hole.

### M9. Image and HTML handling lacks security rules. Lines 155, 206 [D]
* **Description:** No format allowlist, no pixel-count decode limit, and no values for the "size cap" and "set pixel size". Missing: filename scheme and collision handling for dropped files, confinement of relative image paths to `images/` (a crafted `../../` link in a synced note), and a statement that remote `<img src>` loads are disabled. Orphan images are not mentioned.
* **Rationale:** Notes come from a folder other tools or users can modify. Opening a note must not read outside the note folder or make network requests, which would also contradict the no-telemetry stance.

### M10. Bin retention rule and restore rules undefined. Lines 34, 103, 118, 271 [D, T]
* **Description:** Alternative E accepts "a retention rule" that is never defined. ID collision on restore and bin searchability are also unspecified. Deleted notes stay readable in the synced `.bin`.
* **Rationale:** Define retention (indefinite or N days), restore collisions and the privacy implication of the synced bin.

### M11. Plaintext notes in third-party-synced folders is an assertion, not a risk analysis. Lines 28, 58, 303 [D]
* **Description:** "Not requested as a goal" is the only justification, while the spec encourages placing notes in Nextcloud or Google Drive. Sticky notes often hold passwords.
* **Rationale:** Record it as an accepted risk in the Risks section, with user-facing documentation and a pointer to the Future Improvement.

### M12. Search and startup scale unmeasured. Lines 209-210 [D]
* **Description:** The in-memory index needs every note read and parsed at startup. At 10,000 notes this may take seconds and leave the list blank. The origin of the 10,000 figure is not stated, and it is unclear whether search covers titles as well as text.
* **Rationale:** Specify background or lazy indexing, or a plain-text cache in the state dir, or lower the ceiling.

### M13. KDE Frameworks dependency conflicts with the "runs on other desktops" claim. Lines 13, 109, 121-122 [D]
* **Description:** KDBusService and KStatusNotifierItem pull KF6 onto GNOME and Sway and into the Flatpak. Plain QDBus registration covers single instance without KF.
* **Rationale:** State the footprint as an accepted trade-off, give the platform layer a non-KDE path, or record it in Alternatives with a rejection reason.

### M14. Unverified claims in the Problem statement. Lines 17, 19 [T, D]
* **Description:** "Opaque databases", "do not handle images" and "naive port" are subjective, and no apps are named. KNotes is an obvious existing KDE option and is missing from Alternatives.
* **Rationale:** Name the apps surveyed and why each fails, or soften the claim. Consider an Alternative for extending or reviving KNotes.

### M15. "Bing-style insights" is unexplained. Line 44 [T]
* **Description:** A fresh reader cannot tell what is excluded. "User accounts ... are also out" is appended to the cloud-sync bullet.
* **Rationale:** Describe the feature plainly or drop it, and give accounts their own bullet.

### M16. Alternatives section inconsistent and contains meta-commentary. Lines 240, 252, 258, 260-295 [T, S]
* **Description:** Line 240 describes the author's process, not the design. "Reason for Rejection" in B, C and D says the owner chose or a cost is accepted, not a failure condition. Alternatives A-C and H use Pros/Cons/Reason, while D, E, F, G and I use bare option bullets. I has no Reason bullet. D has one combined reason for three options.
* **Rationale:** Use one shape (for example "Reason for rejection and cost accepted"), give each option its own reason, and delete line 240.

### M17. "(owner choice)" tags state who decided, not why. Lines 115-120, 258 [T]
* **Description:** The tag appears on six table rows but not on lines 121-123.
* **Rationale:** Replace with the rationale, or record the decision source once. Consider a Source column.

### M18. Unmeasured performance claims. Lines 250, 252 [T]
* **Description:** "Heavier memory", "slower start", "fastest option, which was a stated need" have no figures or source.
* **Rationale:** Give a target (for example cold start under N ms) or remove the claims.

## Nit

1. Lines 56, 59, 225-232 [D, S, T]: spikes S1-S6 are listed twice with different wording, and only S1 and S4 state pass criteria. Keep one table with decision, pass criterion and fallback if the spike fails.
2. Lines 79, 36: "says so once" does not say once ever or per session, or where the flag is stored.
3. Lines 31, 214: the color token palette and the contrast rule are not named.
4. Lines 4-5, 109, 219: no success metrics, test strategy, accessibility line (frameless windows) or i18n mention.
5. Lines 118, 188: no pruning rule for stale `geometry.json` entries.
6. Lines 297-306: "Future Improvements" is an H3 nested under Alternatives and should be H2. A dangling trailing rule at 305.
7. Lines 263-264: Alternative D option names are not bolded consistently and the geometry option is a compound label. Split it.
8. Lines 162-169, 165: conditional text in lifecycle table cells ("Quit if mode is background") is hard to scan. Spell out "if the user setting is background" or use separate rows.
9. Lines 36, 79, 160, 35, 167: "background mode", "tray mode", "last window" and "list button" are used before they are defined. Define them once, early.
10. Line 39: "Everything above works" is absolute while spikes S2, S3 and S5 are open. Say "is the acceptance target".
11. Lines 27-28: the goals are wordy. Split line 27's six behaviors into a list, and shorten line 28's "overwrite a newer outside edit" wording.
12. Line 41: "Flathub depends on the sandbox allowing ..." is vague. Say "requires".
13. Lines 45, 295: "no confirmed Wayland standard" and the detailed wording at 295 differ. Align or cross-reference.
14. Lines 57, 153: "Markdown cannot store underline" is stated twice.
15. Line 58: "Not requested as a goal" is filler. Say "Notes are stored as plain files and are not encrypted."
16. Lines 109, 114: "KDE Frameworks is allowed for ..." should read "The app uses ..."; line 114 repeats line 109.
17. Line 171: the quit prompt does not list its options (save, discard, cancel).
18. Lines 175, 210: "about 400 ms" and "about 10,000 notes" are imprecise. State a value or target ceiling.
19. Line 194: "therefore needs one watch per note folder, so the watch count grows" is wordy.
20. Line 201: "Spike S2 decides how far it goes" is vague. Say what S2 decides.
21. Line 218: the policy and its example (tray missing) are mixed.
22. Line 223: the no-telemetry mention duplicates the non-goal at line 52.
23. Line 252: "Qt is native on Plasma" is vague. Say Qt is the toolkit Plasma is built on.
24. Line 302: "Moved here to keep the first release small" is editorial history.
25. Lines 11, 17: the Windows Sticky Notes comparison is repeated.
26. Lines 127-148: component labels inconsistent in spacing, for example `NoteIndex(list model + filter)`. Cosmetic.

## Reviewer notes
* Structural found the document otherwise compliant: heading hierarchy, table and code-fence formatting and list numbering are fine. The Alternatives Rule is met. Structural's M1 was rated Major, with a note that it could be a Nit under lenient reading of the standard.
* Style had no standard supplied and used persona defaults.
* The structural persona file is `structural.md`, not `egm-structural.md`.
