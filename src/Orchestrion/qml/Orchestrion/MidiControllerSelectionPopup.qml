import QtQuick 2.15
import QtQuick.Controls 2.15
import Orchestrion

Item {
    Component.onCompleted: {
        midiControllerSelectionModel.init()
    }

    Row {
        x: 10
        y: 10
        spacing: 10

        Button {
            id: button
            text: "Controllers"

            onReleased: {
                if (dropdown.visible) {
                    dropdown.close()
                } else {
                    dropdown.open()
                }
            }

            Popup {
                id: dropdown
                focus: true
                y: button.height + 5
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                padding: 0
                background: Rectangle {
                    id: popupBackground
                    implicitWidth: 200
                    color: Qt.rgba(1, 1, 1, 0.95)
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
                        onCheckedChanged: {
                            midiControllerSelectionModel.updateControllerIsSelected(index, checked)
                        }
                    }
                    model: GestureControllerSelectionModel {
                        id: midiControllerSelectionModel
                    }
                }
            }
        }

        Repeater {
            id: repeater
            model: midiControllerSelectionModel.selectedControllersInfo
            delegate: Rectangle {
                height: button.height
                width: button.height
                color: modelData.isAvailable ? "green" : "grey"
                radius: width / 2
                border.color: "white"
                border.width: 2
                Text {
                    anchors.centerIn: parent
                    text: modelData.shortName
                    font.pixelSize: button.height / 2
                    color: "white"
                }
            }
        }
    }
}