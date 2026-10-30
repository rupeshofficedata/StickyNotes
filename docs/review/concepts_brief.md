# Sticky Notes for Linux (Qt 6 / QML)

**Author/Date:** EGM Synthesizer, 2026-10-28. Mode: BRAINSTORM.
**Inputs:** design, codebase-details, zoom-out, comprehension, implementation-readiness and structural reviewers. The project directory is empty. No reviewer verified Qt, Wayland or Flatpak facts against current docs. Those claims are marked "unverified" and listed as spikes.

## Overview

A standalone sticky-notes app for Linux, built with Qt 6.10+ and QML. Target is KDE Plasma 6.4+ on Wayland, with reduced behavior on GNOME, Sway and X11.

- A primary window lists notes, searches them, and holds settings.
- Each note is its own free-floating window with note colors, bold, italic, underline, strikethrough, bullet lists, images (paste and drag-and-drop), dark mode and delete.
- Every note window has a list button that reopens the primary window.
- Single instance. A relaunch focuses or opens the primary window. A `.desktop` launcher is installed.
- The primary window can be hidden to the system tray, or to the background (process alive, no primary window). In background mode without a tray, closing the last note window quits the app. In tray mode the app stays alive.
- Notes are files in a default folder (`$XDG_DATA_HOME/<app-id>/notes`), changeable in settings. There is no first-run prompt and no `/tmp` option (decided, D1). Window x, y and size are saved in a separate geometry file, not in the note body (decided, D4).
- Wayland `xdg-session-management-v1` is used for position restore where available. It is not a dependency.
- Feature set follows Windows Sticky Notes without cloud sync, accounts or Bing insights.
- Intended distribution: AUR and Flathub.

## Analysis

### Findings that block coding

1. **Lifecycle is undefined.** "Tray", "background" and "hidden" are used loosely. The concept does not say what happens when the primary window closes, or the last note closes, with the primary window hidden or visible, tray present or absent. Background mode can leave a running process the user cannot reach, especially on GNOME where no tray exists by default. Qt's `quitOnLastWindowClosed` does not match the stated rule, so quit must be implemented explicitly.
2. **Note file format is undefined.** Format, how underline/strike/bullets/images persist, note ID versus filename, schema version, handling of unreadable files. Markdown cannot represent underline (codebase reviewer), and Qt's HTML output is verbose and lossy, so the editor choice fixes the format. Files in a user folder become a compatibility contract once anyone has notes.
3. **Geometry in the note file is unsafe as specified.**
   - Every move or resize rewrites a file that also holds the content. A crash mid-write can corrupt it, and a folder synced by Syncthing or Nextcloud will move windows between machines.
   - Wayland clients cannot set or read their own global position, so stored x/y works on X11 only unless session management delivers it. Writing back positions the compositor did not honor would overwrite good data.
4. **Image storage is undefined.** Inline base64 (large files, slow autosave) or an assets folder (breaks single-file copy and sync). Pasted images have no filename. Delete can orphan images. Limits and missing-image behavior are open.
5. **Delete semantics are undefined.** Permanent, XDG trash, or in-app bin; confirmation; undo; image handling; external deletion while a note is open.
6. **The temp-location option is a data-loss path.** `/tmp` is cleared on reboot, and in a Flatpak it is a private tmpfs wiped on restart. On the host it is world-traversable. The term itself is ambiguous (notes folder, scratch space, or image cache).
7. **User-chosen folder versus Flatpak and folder changes.**
   - A folder chosen through the file portal appears as a document-portal path. Whether access survives a restart, and whether file-change events work on that FUSE path, is unverified.
   - Changing the folder in settings is unspecified: move, copy or repoint; what happens to open notes.
   - A missing, read-only or unmounted folder must not silently produce an empty store.
8. **Qt 6.10 floor versus packaging.** Whether the Flathub KDE runtime ships Qt 6.10 is unverified (gating check: `flatpak remote-info flathub org.kde.Platform//6.10`). No reviewer identified a 6.10-only feature the app needs. AUR (rolling Qt) and Flatpak (pinned) may differ.
9. **Silent save failure.** If `QSaveFile::commit()` fails (permissions, vanished folder, full disk), the user keeps typing into a note that is never saved. Failures must surface in the UI, and quitting with unsaved notes must be blocked or prompted.

### Technical notes (codebase-details; all unverified against current docs)

- **Rich text:** QML `TextEdit` has no formatting API. A small C++ helper over `QQuickTextDocument` and `QTextCursor` is needed. The Qt `texteditor` Quick Controls example (`DocumentHandler`) is a reuse candidate. Bullets use `QTextListFormat`. So the app is not pure QML.
- **Images:** `TextEdit` does not accept image paste. Intercept paste in C++ (`QClipboard` mime data), use a `DropArea` for drop, insert via `QTextCursor::insertImage`.
- **Single instance:** no Qt-provided API. Options are `KDBusService` (KF6, gives activation tokens) or registering the app-ID bus name and implementing `org.freedesktop.Application` by hand (no KDE dependency). Pair either with `DBusActivatable=true`. Three reviewers favor D-Bus.
- **Wayland focus:** `requestActivate()` needs an activation token. A second launch may only flash the taskbar entry unless the token is passed through.
- **Tray:** `QSystemTrayIcon` or `Qt.labs.platform.SystemTrayIcon` (deprecation status in 6.10 unverified) use StatusNotifierItem. GNOME needs an extension; Sway needs a bar with a tray. Check availability at runtime and force background mode if missing. Flatpak needs `--talk-name=org.kde.StatusNotifierWatcher`. Persisting with no window in Flatpak may want the Background portal.
- **Session management:** whether Qt 6.10 exposes `xdg-session-management-v1`, whether any per-window API exists, and which KWin version implements it are all unverified. If Qt has no API, the options are a private QtWayland extension or dropping the bonus.
- **Frameless notes:** `startSystemMove()` and `startSystemResize()` work on Wayland. GNOME has no server-side decorations, so a fallback header is needed.
- **Atomic save:** `QSaveFile` needs write access to the directory, does not fsync the directory, and its rename triggers watcher events (filter own writes). Debounce saves (300 to 500 ms) and flush on close, quit and SIGTERM.
- **Dark mode:** `Qt.styleHints.colorScheme` exists since 6.5. Store note colors as tokens that stay readable in both schemes.
- **Diagnostics:** add `QLoggingCategory` categories (storage, tray, session, activation), and a "Copy diagnostics" action in settings (Qt version, platform plugin, desktop, Flatpak, tray availability, protocol bound, folder writable, run mode). Quarantine unparseable note files as `.bad` instead of overwriting. No telemetry.
- **Dependencies:** avoid KDE Frameworks and Kirigami unless a decision is made to accept them; they complicate GNOME and Flathub builds.

### Other findings

- Search: in-memory filter over titles and text is enough. State a ceiling (about 10,000 notes). No database.
- File watching: `QFileSystemWatcher` has inotify limits, rename quirks and own-write echo. Rescan on focus is simpler.
- Missing from the concept: autosave policy, reverse-DNS app ID (shared by `.desktop`, D-Bus name, Flatpak, config path), settings contents, new-note placement and entry points, accessibility, translations, license, backup/export, and a testable definition of "degrades gracefully".
- Always-on-top is correctly out of scope. Docs should point users to compositor window rules.
- Likely first-hour bug reports: "I closed the window and the app vanished"; "I cannot find it (no tray)"; "notes reopen in the wrong place on Wayland"; "I deleted a note and it is gone".

### Platform capability matrix (to be filled by spikes)

| Capability | KDE Wayland | GNOME Wayland | Sway | X11 | Flatpak |
|---|---|---|---|---|---|
| Restore size | expected | expected | expected | expected | expected |
| Restore position | unknown (S2) | no | no | yes | same as host |
| Tray | yes (SNI) | needs extension | needs bar with tray | depends | needs permission |
| Focus on relaunch | unknown | attention hint likely | unknown | yes | unknown |

## Goals vs Non-Goals

**Goals**
- Notes survive crashes, reboots and folder problems. Save failures are visible.
- Every lifecycle state has a defined way to reach and to quit the app.
- Formatting, colors and images round-trip through save and load unchanged.
- Fully working on KDE Plasma 6.4+ Wayland. Elsewhere: notes usable, size restored, differences documented.
- Installable from AUR; Flathub after runtime and portal checks.

**Non-Goals**
- Cloud sync, accounts, Bing insights.
- Always-on-top.
- Dependence on `xdg-session-management-v1`.
- Database or search index (until measured).
- Tags, folders, plugins, Markdown extensions, custom themes, custom color picker.
- Exact window position restore on Wayland.
- A `/tmp` storage option (see D1).
- Remote telemetry.

## Key Decisions & Staging for Next Steps

### Decisions needed (reviewer recommendation in brackets)

| # | Decision | Recommendation | Revisit trigger |
|---|---|---|---|
| D1 | Temp-location option | **DECIDED by owner: dropped.** | A concrete use case |
| D2 | Default folder | **DECIDED by owner:** `$XDG_DATA_HOME/<app-id>/notes` as default, plus a folder entry in settings. No first-run prompt (replaces the earlier first-run-prompt idea). | S3 |
| D3 | Note format | One file per note, version field, stable ID (not filename), unknown fields preserved. HTML versus Markdown decided by S1 (Markdown cannot hold underline). | S1 |
| D4 | Geometry storage | **DECIDED by owner: separate from the note body** (sidecar or one state file; choice still open), debounced, `QSaveFile`, fields optional. Apply x/y only on X11 unless S2 proves otherwise. Never write back unhonored positions. | S2 |
| D5 | Images | Files in an assets folder, relative links, size cap, downscale, orphan cleanup. | S1, S6 |
| D6 | Delete | XDG trash or in-app bin; pick one. | none |
| D7 | Lifecycle | One C++ `AppLifecycle` owner with a state and event table. "Last window" = last visible window of any kind. `quitOnLastWindowClosed=false`. QML sends intents only. Explicit Quit in primary and tray menus. | none |
| D8 | Tray unavailable | Detect at runtime; fall back to background mode with a visible notice. | S5 |
| D9 | Single instance | D-Bus name plus `DBusActivatable=true`; pass activation token. KDBusService versus hand-rolled depends on D14. | S3 |
| D10 | Folder change | Offer to move notes; define conflict policy; never silently create an empty store. | none |
| D11 | External edits | Rescan on focus and manual refresh; watcher deferred. | S4 |
| D12 | Qt floor | **DECIDED by owner: lower it if better.** Use the lowest Qt 6 version that has everything the app needs (Flathub KDE runtime version as the reference) unless a 6.10 feature is named. | S3 |
| D13 | App ID | Fix a reverse-DNS ID before code. | none |
| D14 | KDE Frameworks | **DECIDED by owner: acceptable.** Use KDBusAddons (`KDBusService`: single instance, activation tokens) and KStatusNotifierItem for the tray where they help. Still verify they build and run in the Flatpak (KDE runtime) and degrade on GNOME and Sway. | S3, S5 |
| D16 | New-note placement and color | **DECIDED by owner:** new notes cascade (each offset from the last). Default color is yellow in light theme and black in dark theme. Store the color as a token, not a hex value, so text color flips with the theme. Where Wayland ignores placement, the compositor chooses the position. | none |
| D15 | Save failure | Surface in UI, retry, prompt on quit with dirty notes. | none |

Suggested architecture (structural reviewer): C++ core without QML dependency (`NoteRepository`, `NoteIndex` list model with filter proxy, `NoteDocument`, `AppLifecycle`, `SingleInstance`, `Settings`), a platform layer (`PlatformCaps`, `WindowPlacer`, `TrayIcon`), a `RichTextBridge`, and QML for `PrimaryWindow` and `NoteWindow` only. QML holds no file paths or persistence. Settings live in QSettings, not in the notes folder.

### Scope tension to resolve

The zoom-out reviewer proposed a smaller first release: background mode only, tray as later opt-in, geometry best-effort, AUR only. **Owner decision: tray ships in the first release, together with background mode.** This makes the D7 lifecycle table and the tray spikes (S5) first-release work. Geometry stays best-effort. The distribution scope (AUR only versus AUR plus Flathub) is still open.

### Staging

- **Stage 0, spikes (about half a day each). DECIDED by owner: run all spikes before any app code. Results fill the capability matrix and settle D3, D5, D8, D9, D11, D12, D14:**
  - S1: `TextEdit` rich text with underline, strike, bullets and inline image saved and reloaded; pass if a second save is byte-identical.
  - S2: set and read window position, and session-management support, on KDE 6.4, Sway, GNOME; record what Qt exposes.
  - S3: Flatpak hello-world on the current KDE runtime; record Qt version; folder access through the portal across restart; D-Bus name; KDBusAddons availability.
  - S4: watching 1000 files, atomic rename, own-write echo.
  - S5: tray on KDE, GNOME with extension, Sway; fallback with no host.
  - S6: image paste and drop from Spectacle, Firefox, a file manager; whether C++ is required.
- **Stage 1:** note window, storage, primary list and search, on KDE and X11. Default folder, atomic writes, delete via trash.
- **Stage 2:** lifecycle table implemented, single instance, tray with fallback, quit rules, flush on quit and SIGTERM.
- **Stage 3:** images, settings (folder change flow), dark mode, color palette, diagnostics.
- **Stage 4:** session-management and per-compositor degradation, if S2 supports it.
- **Stage 5:** `.desktop`, metainfo, AUR package; Flatpak manifest after S3.

### Open questions for the owner

Resolved: drop `/tmp` (D1); default folder plus settings entry, no first-run prompt (D2); geometry stored separately (D4); lower Qt floor if better (D12); KDE Frameworks acceptable (D14); cascade placement with yellow (light) and black (dark) default (D16); tray ships in the first release; spikes run first (Stage 0).

Also resolved: first release ships on AUR plus Flathub (Flathub packaging waits on S3 results). Dark-theme default note color is near-black with light text, kept distinct from the window background.

Still open: none from the owner. Open items are now the Stage 0 spike results.
