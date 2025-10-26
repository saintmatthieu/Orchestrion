import QtQuick 2.15
import QtQuick.Controls 2.15
import Orchestrion.OrchestrionSequencer 1.0

Item {
    Component.onCompleted: {
        selectionModel.init()
    }

    width: dropdown.visible ? Math.max(selectedControllerRow.width, dropdown.width) : selectedControllerRow.width
    height: selectedControllerRow.height + (dropdown.visible ? dropdown.height : 0) + 10

    Row {
        id: selectedControllerRow
        spacing: 10

        Button {
            id: button
            text: "Controllers"

            Rectangle {
                id: warningIndicator
                anchors.fill: parent
                radius: 5
                color: "orange"
                opacity: selectionModel.hasWarning ? 0 : 0
                SequentialAnimation on opacity {
                    running: selectionModel.hasWarning
                    loops: Animation.Infinite
                    NumberAnimation { to: 0; duration: 1000 }
                    NumberAnimation { to: 0.2; duration: 1000 }
                }

                ToolTip {
                    id: toolTip
                    visible: selectionModel.hasWarning && button.hovered
                    delay: 1000
                    x: parent.width
                    y: parent.height / 2 - height / 2
                    background: null
                    contentItem: Text {
                        text: qsTr("No active controller, click to select")
                        color: "black"
                        font.pointSize: 10
                        verticalAlignment: Text.AlignTop
                    }
                }
            }

            onReleased: {
                if (dropdown.visible) {
                    dropdown.close()
                } else {
                    dropdown.open()
                }
            }
        }

        Repeater {
            id: repeater
            model: selectionModel.selectedControllersInfo
            delegate: Image {
                visible: !toolTip.visible
                source: modelData.icon
                width: button.height
                height: button.height
                opacity: modelData.isWorking ? 0.7 : 0.2
            }
        }
    }

    Popup {
        id: dropdown
        y: selectedControllerRow.y + selectedControllerRow.height + 10
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        padding: 0
        background: Rectangle {
            id: popupBackground
            implicitWidth: 200
            color: Qt.rgba(1, 1, 1, 0.8)
            implicitHeight: listView.currentItem.height * listView.count + dropdown.padding * 2
        }
        contentItem: ListView {
            id: listView
            width: parent.width
            height: parent.height
            delegate: CheckBox {
                id: checkBox
                text: controllerName
                checked: controllerIsSelected
                opacity: controllerIsWorking ? 1 : 0.5
                onCheckedChanged: {
                    selectionModel.updateControllerIsSelected(index, checked)
                }
                contentItem: Text {
                    leftPadding: 20
                    text: checkBox.text
                    color: "black"
                    font: checkBox.font
                }
            }
            model: GestureControllerSelectionModel {
                id: selectionModel
            }
        }
    }
}