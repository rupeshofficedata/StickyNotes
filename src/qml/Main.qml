import QtQuick
import StickyNotes

// Root object (not a window): holds the primary window and the note window manager,
// so the manager lives while the primary window is hidden.
QtObject {
    id: root

    property NoteWindowManager manager: NoteWindowManager {}
    property PrimaryWindow primary: PrimaryWindow { visible: true }
    property QuitPrompt prompt: QuitPrompt {}

    // AppLifecycle decides; this object only carries out its requests.
    property Connections lifecycle: Connections {
        target: Lifecycle
        function onShowPrimaryRequested() {
            root.primary.show()
            root.primary.raise()
            root.primary.requestActivate()
        }
        function onHidePrimaryRequested() { root.primary.hide() }
        function onOpenNoteRequested(id) { root.manager.openNote(id) }
        function onCloseAllNotesRequested() { root.manager.closeAll() }
        function onQuitBlocked(ids) {
            root.prompt.ids = ids
            root.prompt.show()
            root.prompt.raise()
            root.prompt.requestActivate()
        }
    }
}
