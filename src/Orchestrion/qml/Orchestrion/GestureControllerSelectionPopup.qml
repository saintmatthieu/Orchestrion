import QtQuick 2.15
import QtQuick.Controls 2.15
import Orchestrion

Item {
    Component.onCompleted: {
        gestureControllerSelectionModel.init()
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
                            gestureControllerSelectionModel.updateControllerIsSelected(index, checked)
                        }
                    }
                    model: GestureControllerSelectionModel {
                        id: gestureControllerSelectionModel
                    }
                }
            }
        }

        Repeater {
            id: repeater
            model: gestureControllerSelectionModel.selectedControllersInfo
            delegate: Image {
                source: modelData.icon
                width: button.height
                height: button.height
                opacity: modelData.isWorking ? 0.7 : 0.2
            }
        }

        Image {
            source: gestureControllerSelectionModel.warningIcon
            width: button.height
            height: button.height
            opacity: 0.7
            visible: gestureControllerSelectionModel.hasWarning
        }
    }
}