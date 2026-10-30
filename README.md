# StickyNotes

Sticky notes for KDE Plasma on Wayland. Each note is a small window with colors, formatting and images. A list window finds, searches and manages all notes.

Status: version 0.1.0. It is not published yet. The design is in [docs/designs/sticky_notes.md](docs/designs/sticky_notes.md).

## Features

* Notes in separate windows: bold, italic, underline, strikethrough, bullet lists, four note colors.
* Paste or drag images into a note.
* A list window with search, a Bin, and settings. Deleted notes stay in the Bin until you empty it.
* Dark mode follows the system.
* Closing the list window hides it. With a system tray the app keeps running. Without a tray it quits when the last window closes.
* One instance: starting the app again shows the list window.

## Install

Not on the AUR or Flathub yet. To build from source you need Qt 6.10 or newer, KDE Frameworks 6 (`kdbusaddons`, `kstatusnotifieritem`), CMake and a C++17 compiler. On Arch:

```sh
sudo pacman -S qt6-base qt6-declarative kdbusaddons kstatusnotifieritem cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/stickynotes
```

The Qt 6.10 floor comes from KDE Frameworks 6.31, which requires it. An older KDE Frameworks release may allow an older Qt, but that is untested.

## Where notes are stored

Notes are plain files. The default folder is `~/.local/share/io.github.rupeshofficedata.StickyNotes/notes`. Each note is a folder named by a UUID:

```text
<notes folder>/<uuid>/note.html   text and formatting
<notes folder>/<uuid>/meta.json   the color
<notes folder>/<uuid>/images/     pasted and dropped images (PNG)
<notes folder>/.bin/              deleted notes
```

The app never deletes the notes folder. Notes are not encrypted.

## Change the notes folder

Open the Settings tab and choose a folder. The app does not copy or move anything. Notes in the old folder stay where they are, and the new folder starts empty unless it already holds notes. To move your notes, use your file manager, then press Refresh. The "Open notes folder" button opens the folder in your file manager.

## Use a synced folder for backup

You can point the app at a folder that Nextcloud, Google Drive (through a sync client or an rclone mount) or Syncthing already syncs. The app does not sync by itself.

* When the app becomes active again, or when you press Refresh, it rescans the folder.
* A note that is open and has no unsaved edits reloads on its own when the file changed.
* A note with unsaved edits shows "Changed outside this app" with Reload (discard your edits) and Keep mine (your next save overwrites the file).
* Notes do not update live. A note left open on the desktop updates when you come back to the app.

## What works on each desktop

| | KDE Plasma 6 Wayland | GNOME, Sway and others |
|---|---|---|
| Notes, formatting, images, search, Bin | yes | yes |
| Window size is remembered | yes | yes |
| Window position is remembered | no (Wayland does not allow it) | X11 only |
| System tray | yes | needs a StatusNotifier host (GNOME needs the AppIndicator extension) |
| Without a tray | not applicable | the app quits when you close the last window, and tells you once |

New notes open at a cascading position on X11. On Wayland the compositor decides where they appear.

## Keep a note on top

The app has no always-on-top option, because Wayland has no standard way to ask for it. In KDE, focus a note, press Alt+F3, and choose More Actions, then Keep Above. For a permanent rule, use System Settings, Window Management, Window Rules, and match the window class `stickynotes`.

## Limits

* Up to 400 ms of typing can be lost in a crash, and a power loss can lose the last save.
* If a save fails, the note shows the reason and keeps your text. Quitting while a save is failing asks you to retry or discard.
* Images over 10 MB are rejected. Images are copied to PNG.
* Tested only on Plasma 6 Wayland so far.

## Report a bug

Include your distribution, desktop and version, the app version, the Qt version, and the output of:

```sh
QT_LOGGING_RULES="stickynotes.*=true" QT_FORCE_STDERR_LOGGING=1 stickynotes
```

## Develop

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests run without a display. See [src/README.md](src/README.md) for the code layout.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
