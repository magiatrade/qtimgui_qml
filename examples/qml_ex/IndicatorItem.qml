import QtQuick
import QtQuick.Controls
import QtQuick.Window

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

    // Drag properties
    Drag.active: dragArea.drag.active
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2
    Drag.mimeData: { "indicatorType": indicatorType }
    Drag.dragType: Drag.Automatic

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

    // Store original parent for restoration
    property Item originalParent: null

    // Mouse area for dragging
    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true

        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

        property point startPos: Qt.point(0, 0)

        onPressed: function(mouse) {
            // Save original position and parent
            startPos = Qt.point(root.x, root.y)
            root.originalParent = root.parent

            // Map to window coordinates before parent change
            var globalPos = root.mapToItem(null, 0, 0)
            root.x = globalPos.x
            root.y = globalPos.y

            root.grabToImage(function(result) {
                root.Drag.imageSource = result.url
            })
            root.Drag.active = true
        }

        onReleased: {
            root.Drag.drop()
            root.Drag.active = false
        }

        onPositionChanged: function(mouse) {
            if (pressed) {
                // Update position in window coordinates
                var globalPos = mapToItem(null, mouse.x, mouse.y)
                root.x = globalPos.x - root.width / 2
                root.y = globalPos.y - root.height / 2
            }
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
            when: dragArea.pressed
            PropertyChanges {
                target: root
                scale: 1.1
                opacity: 0.8
                z: 1000
            }
            ParentChange {
                target: root
                parent: root.Window.contentItem
            }
        }
    ]

    transitions: Transition {
        from: "dragging"
        to: ""
        SequentialAnimation {
            ScriptAction {
                script: {
                    // Restore to original parent and position
                    if (root.originalParent) {
                        root.parent = root.originalParent
                        root.x = dragArea.startPos.x
                        root.y = dragArea.startPos.y
                    }
                }
            }
        }
    }
}
