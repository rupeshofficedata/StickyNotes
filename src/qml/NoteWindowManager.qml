import QtQuick
import StickyNotes

// Creates one NoteWindow per open note. Lives in Main.qml, so it outlives the primary window.
QtObject {
    id: mgr

    property var windows: ({})
    property Component noteComponent: Component { NoteWindow {} }

    function openNote(id) {
        const existing = windows[id]
        if (existing) {
            existing.show()
            existing.raise()
            existing.requestActivate()
            return
        }
        const pos = Lifecycle.nextCascadePosition()
        const w = noteComponent.createObject(null, { noteId: id, x: pos.x, y: pos.y })
        windows[id] = w
        w.finished.connect(function () {
            delete mgr.windows[id]
            w.destroy(100)
        })
        w.show()
    }

    function closeAll() {
        for (const id of Object.keys(windows))
            windows[id].close()   // each window saves before it closes
    }
}
