import QtQuick
import QtQuick.Controls
import qml_ex

Window {
    id: root
    width: 1280
    height: 720
    visible: true
    title: qsTr("QtImGui QML Example")
    color: "#1e1e1e"

    // ImGui rendering area - fills the entire window
    ImGuiQuickItem {
        id: imguiItem
        anchors.fill: parent
        focus: true
    }

    // Optional: QML overlay on top of ImGui
    Text {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        text: "QML Overlay - Qt " + Qt.version
        color: "#888888"
        font.pixelSize: 12
    }
}
