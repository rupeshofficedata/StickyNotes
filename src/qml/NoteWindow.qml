import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StickyNotes

// One sticky note. Frameless, so it draws its own header (drag area, list button, delete button).
Window {
    id: win

    property string noteId
    property string colorToken: "default"
    signal finished()

    property bool loading: true
    property bool dirty: false
    property string lastHtml
    property string lastColor: "default"

    width: 280
    height: 320
    minimumWidth: 200
    minimumHeight: 160
    flags: Qt.Window | Qt.FramelessWindowHint
    color: Theme.background(colorToken)
    title: qsTr("Note")

    // force = true writes even when nothing changed, for "Recreate" after an outside removal.
    function save(force) {
        if (!dirty && !force)
            return
        const html = bridge.html()
        if (!force && html === lastHtml && colorToken === lastColor) {
            dirty = false
            return
        }
        if (NoteStore.save(noteId, html, colorToken)) {
            lastHtml = html
            lastColor = colorToken
            dirty = false
            banner.mode = ""
            banner.message = ""
        }
    }

    // Replaces the text with the file on disk and discards unsaved edits.
    function reload() {
        loading = true
        const d = NoteStore.load(noteId)
        colorToken = d.colorToken
        lastColor = d.colorToken
        edit.text = bridge.sanitizeHtml(d.html)
        lastHtml = bridge.html()
        dirty = false
        banner.mode = ""
        banner.message = d.ok ? "" : qsTr("Cannot read this note: %1").arg(d.error)
        NoteStore.trackOpen(noteId)
        loading = false
    }

    function remove() {
        dirty = true
        save()
        if (NoteStore.moveToBin(noteId)) {
            NoteIndex.rescan()
            close()
        }
    }

    onClosing: {
        saveTimer.stop()
        save()
        NoteStore.saveWindowState(noteId, width, height, x, y, Qt.platform.pluginName === "xcb")
        NoteStore.untrack(noteId)
        NoteIndex.rescan()
        finished()
    }

    onVisibleChanged: visible ? Lifecycle.windowShown("note", noteId) : Lifecycle.windowHidden("note", noteId)

    Component.onCompleted: {
        // Size comes back everywhere. Position only on X11; Wayland ignores it.
        const st = NoteStore.windowState(noteId)
        if (st.width) width = st.width
        if (st.height) height = st.height
        if (st.x !== undefined && Qt.platform.pluginName === "xcb") { x = st.x; y = st.y }
        reload()
    }

    Timer {
        id: saveTimer
        interval: 400
        onTriggered: win.save()
    }

    // Retry a failed save while the note stays open.
    Timer {
        interval: 10000
        repeat: true
        running: win.dirty && banner.mode === "saveFailed"
        onTriggered: win.save()
    }

    Connections {
        target: NoteStore
        // Quit asks every window to save now.
        function onFlushRequested() {
            saveTimer.stop()
            win.save()
        }
        function onSaveFailed(id, reason) {
            if (id !== win.noteId)
                return
            banner.mode = "saveFailed"
            banner.actions = []
            banner.message = qsTr("Save failed: %1. Your text is kept; retrying.").arg(reason)
        }
        function onImageRejected(id, reason) {
            if (id !== win.noteId)
                return
            banner.mode = "image"
            banner.actions = []
            banner.message = reason
        }
        // A sync client or another program changed the files. Quiet when there is nothing to lose.
        function onNoteChangedOutside(id) {
            if (id !== win.noteId)
                return
            if (!win.dirty) {
                win.reload()
            } else {
                banner.mode = "changed"
                banner.actions = [qsTr("Reload"), qsTr("Keep mine")]
                banner.message = qsTr("Changed outside this app.")
            }
        }
        function onNoteRemovedOutside(id) {
            if (id !== win.noteId)
                return
            if (!win.dirty) {
                win.close()
            } else {
                banner.mode = "removed"
                banner.actions = [qsTr("Recreate note")]
                banner.message = qsTr("Removed outside this app.")
            }
        }
    }

    RichTextBridge {
        id: bridge
        document: edit.textDocument
        store: NoteStore
        noteId: win.noteId
        cursorPosition: edit.cursorPosition
        selectionStart: edit.selectionStart
        selectionEnd: edit.selectionEnd
        onContentsChanged: {
            if (win.loading)
                return
            win.dirty = true
            saveTimer.restart()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Qt.rgba(0, 0, 0, Theme.dark ? 0.25 : 0.08)

            DragHandler {
                target: null
                onActiveChanged: if (active) win.startSystemMove()
            }
            RowLayout {
                anchors.fill: parent
                spacing: 0
                ToolButton { text: "≡"; onClicked: Lifecycle.showPrimary(); ToolTip.text: qsTr("Show all notes"); ToolTip.visible: hovered }
                Item { Layout.fillWidth: true }
                ToolButton { text: qsTr("Delete"); font.pixelSize: 11; onClicked: win.remove() }
                ToolButton { text: "✕"; onClicked: win.close() }
            }
        }

        ErrorBanner {
            id: banner
            objectName: "banner"
            property string mode
            Layout.fillWidth: true
            onActionTriggered: (index) => {
                if (mode === "changed" && index === 0) {
                    win.reload()                 // Reload: discards unsaved edits
                } else if (mode === "removed") {
                    win.save(true)               // Recreate: writes folder, meta.json and note.html again
                } else {
                    message = ""                 // Keep mine: the next save overwrites the file
                    mode = ""
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            Layout.margins: 2
            ToolButton { text: "B"; font.bold: true; checked: bridge.bold; onClicked: bridge.toggleBold() }
            ToolButton { text: "I"; font.italic: true; checked: bridge.italic; onClicked: bridge.toggleItalic() }
            ToolButton { text: "U"; font.underline: true; checked: bridge.underline; onClicked: bridge.toggleUnderline() }
            ToolButton { text: "S"; font.strikeout: true; checked: bridge.strike; onClicked: bridge.toggleStrike() }
            ToolButton { text: "•"; checked: bridge.bullet; onClicked: bridge.toggleBullet() }
            Repeater {
                model: Theme.tokens
                delegate: Rectangle {
                    required property string modelData
                    width: 18; height: 18; radius: 9
                    color: Theme.background(modelData)
                    border.width: win.colorToken === modelData ? 2 : 1
                    border.color: Theme.foreground()
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            win.colorToken = parent.modelData
                            win.dirty = true
                            saveTimer.restart()
                        }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextEdit {
                id: edit
                objectName: "edit"
                width: parent.width
                padding: 8
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                color: Theme.foreground()
                selectedTextColor: color
                baseUrl: NoteStore.noteUrl(win.noteId)

                Keys.onPressed: (event) => {
                    if (event.matches(StandardKey.Paste)) {
                        bridge.pasteFromClipboard()
                        event.accepted = true
                    }
                }
            }
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (!drop.hasUrls)
                return
            for (let i = 0; i < drop.urls.length; ++i)
                bridge.insertImageFile(drop.urls[i])
        }
    }

    Rectangle {
        width: 14; height: 14
        anchors { right: parent.right; bottom: parent.bottom }
        color: "transparent"
        DragHandler {
            target: null
            onActiveChanged: if (active) win.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        }
        Label { anchors.centerIn: parent; text: "◢"; font.pixelSize: 9; opacity: 0.5; color: Theme.foreground() }
    }
}
