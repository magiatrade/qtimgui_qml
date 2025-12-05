import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qml_ex

ApplicationWindow {
    id: root
    width: 1400
    height: 800
    visible: true
    title: qsTr("QtImGui + QML - Drag & Drop Chart Demo")
    color: "#1a1a2e"

    // Main layout
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left Toolbar
        Rectangle {
            id: toolbar
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: "#16213e"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 8
                anchors.bottomMargin: 32  // Space for status bar
                spacing: 8

                // Header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#0f3460"
                    radius: 8

                    Text {
                        anchors.centerIn: parent
                        text: "Indicators"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#e94560"
                    }
                }

                // Instructions
                Text {
                    Layout.fillWidth: true
                    text: "Drag indicators to the chart area"
                    font.pixelSize: 10
                    color: "#888"
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }

                // Indicator items
                IndicatorItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    indicatorType: "Line"
                    indicatorIcon: "~"
                    indicatorName: "Line Chart"
                    indicatorColor: "#1f77b4"
                }

                IndicatorItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    indicatorType: "Bars"
                    indicatorIcon: "|"
                    indicatorName: "Bar Chart"
                    indicatorColor: "#ff7f0e"
                }

                IndicatorItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    indicatorType: "Scatter"
                    indicatorIcon: "o"
                    indicatorName: "Scatter Plot"
                    indicatorColor: "#2ca02c"
                }

                IndicatorItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    indicatorType: "Shaded"
                    indicatorIcon: "^"
                    indicatorName: "Area Chart"
                    indicatorColor: "#9467bd"
                }

                IndicatorItem {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    indicatorType: "Candles"
                    indicatorIcon: "#"
                    indicatorName: "Candlestick"
                    indicatorColor: "#d62728"
                }

                // Spacer
                Item {
                    Layout.fillHeight: true
                }

                // Active series list with scroll
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(activeSeries.contentHeight + 30, 150)
                    color: "#0f3460"
                    radius: 8
                    visible: chartManager.seriesCount > 0
                    clip: true

                    Column {
                        id: activeSeries
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 4

                        property real contentHeight: childrenRect.height

                        Text {
                            text: "Active Series"
                            font.pixelSize: 11
                            font.bold: true
                            color: "#e94560"
                        }

                        Repeater {
                            model: chartManager.seriesInfo

                            Rectangle {
                                width: activeSeries.width
                                height: 24
                                radius: 4
                                color: modelData.visible ? modelData.color : "#333"
                                opacity: modelData.visible ? 1.0 : 0.5

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        font.pixelSize: 9
                                        color: "white"
                                        elide: Text.ElideRight
                                    }

                                    // Toggle visibility button
                                    Rectangle {
                                        width: 16
                                        height: 16
                                        radius: 3
                                        color: modelData.visible ? "#4CAF50" : "#666"

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.visible ? "V" : "-"
                                            font.pixelSize: 10
                                            color: "white"
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: chartManager.toggleVisibility(modelData.index)
                                        }
                                    }

                                    // Remove button
                                    Rectangle {
                                        width: 16
                                        height: 16
                                        radius: 3
                                        color: "#e74c3c"

                                        Text {
                                            anchors.centerIn: parent
                                            text: "X"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: "white"
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: chartManager.removeIndicator(modelData.index)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Clear button
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    text: "Clear All"
                    visible: chartManager.seriesCount > 0

                    background: Rectangle {
                        color: parent.pressed ? "#c0392b" : parent.hovered ? "#e74c3c" : "#c0392b"
                        radius: 6
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: chartManager.clearAll()
                }
            }
        }

        // Main chart area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0f0f23"

            // Drop area indicator
            Rectangle {
                id: dropIndicator
                anchors.fill: parent
                anchors.margins: 10
                color: "transparent"
                border.color: imguiItem.containsDrag ? "#e94560" : "transparent"
                border.width: 3
                radius: 8

                Behavior on border.color {
                    ColorAnimation { duration: 200 }
                }
            }

            // ImGui rendering area
            ImGuiQuickItem {
                id: imguiItem
                anchors.fill: parent
                focus: true

                property bool containsDrag: false
                property string draggedType: ""
            }

            // Drop area on top of ImGui
            DropArea {
                id: chartDropArea
                anchors.fill: parent

                onEntered: function(drag) {
                    imguiItem.containsDrag = true
                    console.log("Drag entered, source:", drag.source)
                }

                onExited: {
                    imguiItem.containsDrag = false
                    console.log("Drag exited")
                }

                onDropped: function(drop) {
                    imguiItem.containsDrag = false
                    console.log("Dropped, source:", drop.source)
                    if (drop.source && drop.source.indicatorType) {
                        var type = drop.source.indicatorType
                        console.log("Adding indicator:", type, "at", drop.x, drop.y)
                        chartManager.addIndicator(type, drop.x, drop.y)
                    }
                }
            }

            // Drop hint text
            Text {
                anchors.centerIn: parent
                text: imguiItem.containsDrag ? "Release to add indicator" :
                      (chartManager.seriesCount === 0 ? "Drag indicators here" : "")
                font.pixelSize: 24
                color: "#444"
                visible: imguiItem.containsDrag || chartManager.seriesCount === 0
            }
        }
    }

    // Status bar
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 24
        color: "#0f3460"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 4

            Text {
                text: "Series: " + chartManager.seriesCount
                font.pixelSize: 11
                color: "#888"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "QtImGui QML Demo"
                font.pixelSize: 11
                color: "#666"
            }
        }
    }
}
