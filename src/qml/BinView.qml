import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StickyNotes

// Deleted notes. They stay until the user empties the bin. The bin is not searched.
Item {
    id: root

    property string pendingName    // bin entry waiting for "Delete forever" confirmation
    property bool confirmEmpty: false

    function refresh() { BinIndex.rescan(); NoteIndex.rescan() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: qsTr("%n note(s) in the bin", "", list.count) }
            Button { text: qsTr("Empty bin"); enabled: list.count > 0; onClicked: root.confirmEmpty = true }
        }

        ErrorBanner {
            id: banner
            Layout.fillWidth: true
            actions: [qsTr("OK")]
            onActionTriggered: message = ""
        }

        Label {
            Layout.fillWidth: true
            visible: list.count === 0
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("The bin is empty")
            opacity: 0.6
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: BinIndex

            delegate: Rectangle {
                id: row
                required property string noteId     // the bin entry name
                required property string title
                required property string snippet
                required property string colorToken

                width: ListView.view.width
                height: 58
                radius: 4
                color: Theme.background(colorToken)

                RowLayout {
                    anchors { fill: parent; margins: 6 }
                    Column {
                        Layout.fillWidth: true
                        Label { width: parent.width; text: row.title; elide: Text.ElideRight; font.bold: true; color: Theme.foreground() }
                        Label { width: parent.width; text: row.snippet; elide: Text.ElideRight; font.pixelSize: 11; color: Theme.foreground(); opacity: 0.8 }
                    }
                    // Restore needs no confirmation.
                    Button { text: qsTr("Restore"); onClicked: { NoteStore.restoreFromBin(row.noteId); root.refresh() } }
                    Button { text: qsTr("Delete forever"); onClicked: root.pendingName = row.noteId }
                }
            }
        }
    }

    Connections {
        target: NoteStore
        function onRestoreRefused(name, reason) { banner.message = reason }
    }

    Dialog {
        anchors.centerIn: parent
        modal: true
        title: qsTr("Delete forever?")
        visible: root.pendingName.length > 0
        // Cancel is the default button.
        standardButtons: Dialog.Cancel | Dialog.Ok
        Component.onCompleted: standardButton(Dialog.Cancel).highlighted = true
        Label { text: qsTr("This note is removed from the disk and cannot be restored.") }
        onAccepted: { NoteStore.deleteForever(root.pendingName); root.pendingName = ""; root.refresh() }
        onRejected: root.pendingName = ""
    }

    Dialog {
        anchors.centerIn: parent
        modal: true
        title: qsTr("Empty the bin?")
        visible: root.confirmEmpty
        standardButtons: Dialog.Cancel | Dialog.Ok
        Component.onCompleted: standardButton(Dialog.Cancel).highlighted = true
        Label { text: qsTr("All notes in the bin are removed from the disk and cannot be restored.") }
        onAccepted: { NoteStore.emptyBin(); root.confirmEmpty = false; root.refresh() }
        onRejected: root.confirmEmpty = false
    }
}
