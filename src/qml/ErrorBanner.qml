import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Message plus a list of action labels. The caller reacts to actionTriggered(index, label).
Rectangle {
    id: root
    property string message
    property var actions: []
    signal actionTriggered(int index, string label)

    visible: message.length > 0
    implicitHeight: visible ? row.implicitHeight + 8 : 0
    color: "#B3261E"

    RowLayout {
        id: row
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 6 }
        Label {
            Layout.fillWidth: true
            text: root.message
            color: "white"
            wrapMode: Text.Wrap
            font.pixelSize: 11
        }
        Repeater {
            model: root.actions
            delegate: ToolButton {
                required property int index
                required property string modelData
                text: modelData
                font.pixelSize: 11
                onClicked: root.actionTriggered(index, modelData)
            }
        }
    }
}
