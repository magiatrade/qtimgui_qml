import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string indicatorType: "Line"
    property string indicatorIcon: "~"
    property string indicatorName: "Indicator"
    property color indicatorColor: "#4a9eff"

    implicitWidth: 160
    implicitHeight: 50
    radius: 8
    color: dragArea.pressed ? Qt.darker(indicatorColor, 1.3) :
           dragArea.containsMouse ? Qt.lighter(indicatorColor, 1.1) : indicatorColor

    // Drag properties - use Internal for WebAssembly compatibility
    Drag.active: dragArea.drag.active
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
    Drag.dragType: Drag.Internal

    // Visual content
    Row {
        anchors.centerIn: parent
        spacing: 8

        Text {
            text: indicatorIcon
            font.pixelSize: 20
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: indicatorName
                font.pixelSize: 12
                font.bold: true
                color: "white"
            }

            Text {
                text: indicatorType
                font.pixelSize: 10
                color: Qt.rgba(1, 1, 1, 0.7)
            }
        }
    }

    // Mouse area for dragging
    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true
        drag.target: root

        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

        property point startPos: Qt.point(0, 0)

        onPressed: function(mouse) {
            startPos = Qt.point(root.x, root.y)
        }

        onReleased: function(mouse) {
            // Perform the drop
            root.Drag.drop()
            // Reset position
            root.x = startPos.x
            root.y = startPos.y
        }
    }

    // Shadow effect
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: parent.radius + 2
        color: "transparent"
        border.color: dragArea.drag.active ? "#ffffff" : "transparent"
        border.width: 2
        z: -1
    }

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    Behavior on scale {
        NumberAnimation { duration: 150 }
    }

    states: [
        State {
            name: "dragging"
            when: dragArea.drag.active
            PropertyChanges {
                target: root
                scale: 1.05
                opacity: 0.9
            }
        }
    ]
}
