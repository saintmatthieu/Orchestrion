import QtQuick 2.0
import Orchestrion.Test 1.0
Window
{
    id: window
    visible: true
    width: 640
    height: 480
    title: qsTr("Minimal Qml")

    TouchpadTestappModel {
        id: model
    }

    Component.onCompleted: {
        model.init()
    }

    QtObject {
        id: prv
        readonly property var colors: ["red", "green", "blue"]
    }

    Repeater {
        id: repeater
        model: model.contacts

        Rectangle {
            width: 30
            height: 30
            x: modelData.x * window.width - width / 2
            y: modelData.y * window.height - height / 2
            color: prv.colors[modelData.uid % prv.colors.length]
            radius: width / 2
        }
    }
}