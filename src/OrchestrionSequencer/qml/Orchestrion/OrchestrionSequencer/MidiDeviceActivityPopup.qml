import QtQuick 2.15
import QtQuick.Controls 2.15
import Orchestrion.OrchestrionSequencer 1.0

Popup {
  id: root

  MidiDeviceActivityPopupModel {
    id: model
    onShowPopup: {
      root.visible = true
      root.open()
    }
  }

  Component.onCompleted: {
    model.init()
  }

  modal: true
  focus: true
  contentItem: Column {
    padding: 10
    spacing: 10
    Text {
      text: qsTr("Activity detected on MIDI device. Use it as controller?")
      wrapMode: Text.Wrap
      horizontalAlignment: Text.AlignLeft
    }
    Row {
      spacing: 10
      anchors.horizontalCenter: parent.horizontalCenter
      Button {
        function accept() {
          model.accept()
          root.close()
        }
        text: qsTr("Yes")
        onClicked: accept()
        Keys.onReturnPressed: accept()
        Keys.onEnterPressed: accept()
        Keys.onSpacePressed: accept()
        focus: true
      }
      Button {
        id: cancelButton
        function cancel() {
          model.reject()
          root.close()
        }
        text: qsTr("No")
        onClicked: reject()
        Keys.onReturnPressed: reject()
        Keys.onEnterPressed: reject()
        Keys.onSpacePressed: reject()
      }
    }
  }
}