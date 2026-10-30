import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StickyNotes

// Shown when a save failed and the user tries to quit.
Window {
    id: win

    property var ids: []

    width: 360
    height: 150
    title: qsTr("Unsaved notes")
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    color: palette.window

    ColumnLayout {
        anchors { fill: parent; margins: 12 }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("%n note(s) could not be saved. Quitting now loses the unsaved text.", "", win.ids.length)
        }
        Item { Layout.fillHeight: true }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            Button { text: qsTr("Retry save"); onClicked: { win.close(); Lifecycle.quit() } }
            Button { text: qsTr("Discard and quit"); onClicked: { win.close(); Lifecycle.quit(true) } }
            Button { text: qsTr("Cancel"); onClicked: win.close() }
        }
    }
}
