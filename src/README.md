# StickyNotes source

QML draws the windows and sends intents. A C++ core owns notes on disk, the note list and the app lifecycle. `main.cpp` wires them together with the KDE Frameworks pieces.

## Interface

* [`core/`](./core): `NoteStore`, `NoteIndex`, `AppLifecycle`, `PlatformCaps`, `Logging.h`. No Qt Quick.
* [`editor/`](./editor): `RichTextBridge`. Uses Qt Quick text documents.
* [`qml/`](./qml): `Main`, `PrimaryWindow`, `NoteWindow`, `NoteWindowManager`, `BinView`, `SettingsPage`, `QuitPrompt`, `ErrorBanner`, `Theme`.
* `main.cpp`: creates `QApplication`, `KDBusService` (single instance), the optional `KStatusNotifierItem` (tray), the signal bridge for SIGTERM and SIGINT, and registers the singletons `NoteStore`, `NoteIndex`, `BinIndex`, `Lifecycle` for QML.

## Invariants

* Only `AppLifecycle` hides, shows or quits. QML reports `windowShown` and `windowHidden` and asks for actions.
* QML holds no file paths or storage logic. It calls `NoteStore`.
* Note ids are lowercase UUIDs. `NoteStore` rejects any other id, so a QML string cannot escape the notes folder.
* A note is saved as `meta.json` first and `note.html` second, each through `QSaveFile`.
* A missing user-chosen notes folder is an error. Only the default folder is created.
* Tray presence is checked once, at startup.
* Only `images/<uuid>.png` inside the note folder loads into a note. Remote URLs never load.

## Side effects

* Writes notes under the notes folder, and window size (and X11 position) and the folder path into `QSettings`.
* Registers the D-Bus name `io.github.rupeshofficedata.StickyNotes`. A second launch exits with status 0.
* Installs handlers for SIGTERM and SIGINT.
* Rescans the notes folder when the application becomes active.

## Verification

* Run: `ctest --test-dir build --output-on-failure`
