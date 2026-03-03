import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
  id: root
  visible: true
  width: 1100
  height: 720
  title: "S32-Media-Hub"
  color: "#121212"

  function formatMs(ms) {
    if (ms <= 0) return "--:--"
    var total = Math.floor(ms / 1000)
    var m = Math.floor(total / 60)
    var s = total % 60
    return m + ":" + (s < 10 ? "0" + s : s)
  }

  Component.onCompleted: {
    libraryController.loadSaved()
  }

  Connections {
    target: playbackController
    function onCurrentPathChanged() {
      if (playbackController.currentPath.length > 0) {
        videoController.openAndPlay(playbackController.currentPath)
        var title = playbackController.currentTitle
        if (title.length === 0) {
          title = playbackController.currentPath
        }
        hardwareController.displayText(title, playbackController.currentArtist)
      }
    }
    function onPlayingChanged() {
      if (playbackController.playing) {
        videoController.play()
      } else {
        videoController.pause()
      }
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 16
    spacing: 12

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      TextField {
        id: pathField
        Layout.fillWidth: true
        placeholderText: "/path/to/music"
        text: settingsController.lastLibraryPath
      }

      Button {
        text: "Scan"
        onClicked: {
          libraryController.scan(pathField.text)
          settingsController.lastLibraryPath = pathField.text
          settingsController.save()
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      TextField {
        id: searchField
        Layout.fillWidth: true
        placeholderText: "Search title, artist, album..."
        onTextChanged: libraryController.filterText = text
      }

      Label {
        text: libraryController.trackCount + " tracks"
        color: "#aaaaaa"
      }

      Button {
        text: "Shuffle"
        onClicked: playbackController.shuffle()
      }

      Button {
        text: "Resume"
        enabled: settingsController.lastTrackPath.length > 0
        onClicked: {
          playbackController.openAndPlay(settingsController.lastTrackPath)
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      TextField {
        id: serialPortField
        Layout.fillWidth: true
        placeholderText: "/dev/ttyUSB0"
        text: settingsController.serialPort
      }
      TextField {
        id: baudField
        width: 120
        placeholderText: "115200"
        text: settingsController.serialBaud
        inputMethodHints: Qt.ImhDigitsOnly
      }
      Button {
        text: "Save Serial"
        onClicked: {
          settingsController.serialPort = serialPortField.text
          var baud = parseInt(baudField.text)
          settingsController.serialBaud = (isNaN(baud) || baud <= 0) ? 115200 : baud
          settingsController.save()
          hardwareController.setSerialConfig(settingsController.serialPort, settingsController.serialBaud)
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 12

      Rectangle {
        Layout.preferredWidth: 420
        Layout.preferredHeight: 240
        color: "#101010"
        border.color: "#333333"

        Image {
          id: videoView
          anchors.fill: parent
          fillMode: Image.PreserveAspectFit
          source: "image://video/frame?ts=" + videoController.frameCounter
          visible: videoController.active
        }

        Text {
          anchors.centerIn: parent
          text: videoController.active ? "" : "No Video"
          color: "#666666"
        }
      }

      ListView {
        id: trackList
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: libraryController.tracks
        delegate: Rectangle {
          width: trackList.width
          height: 56
          color: ListView.isCurrentItem ? "#2a2a2a" : "#1b1b1b"
          border.color: "#333333"

          Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
            spacing: 2
            width: parent.width - 24

            Text {
              text: model.display
              color: "#e0e0e0"
              font.pixelSize: 14
              elide: Text.ElideRight
              width: parent.width
            }

            Text {
              text: (model.artist ? model.artist : "Unknown Artist") + " • " +
                    (model.album ? model.album : "Unknown Album") + " • " +
                    formatMs(model.durationMs || 0)
              color: "#9a9a9a"
              font.pixelSize: 12
              elide: Text.ElideRight
              width: parent.width
            }
          }

          MouseArea {
            anchors.fill: parent
            onClicked: {
              trackList.currentIndex = index
              playbackController.openAndPlay(model.path)
              settingsController.lastTrackPath = model.path
              settingsController.save()
            }
          }
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Button {
        text: playbackController.playing ? "Pause" : "Play"
        onClicked: playbackController.playPause()
      }
      Button {
        text: "Stop"
        onClicked: {
          playbackController.stop()
          videoController.stop()
        }
      }
      Button {
        text: "Next"
        onClicked: playbackController.next()
      }

      Slider {
        id: posSlider
        Layout.fillWidth: true
        from: 0
        to: playbackController.durationMs > 0 ? playbackController.durationMs : 1
        value: playbackController.positionMs
        enabled: playbackController.durationMs > 0
        onReleased: {
          playbackController.seek(value)
          videoController.seek(value)
        }
      }

      Label {
        text: formatMs(playbackController.positionMs) + " / " + formatMs(playbackController.durationMs)
        color: "#888888"
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8

      Label {
        text: "Volume"
        color: "#d0d0d0"
      }
      Slider {
        id: volumeSlider
        Layout.fillWidth: true
        from: 0
        to: 1
        stepSize: 0.01
        value: playbackController.volume
        onMoved: {
          playbackController.setVolume(value)
          settingsController.volume = value
        }
        onReleased: settingsController.save()
      }
    }

    RowLayout {
      Layout.fillWidth: true
      spacing: 8
      Label {
        text: playbackController.positionMs + " / " + playbackController.durationMs + " ms"
        color: "#888888"
      }
      Item { Layout.fillWidth: true }
      Button {
        text: "Mock: Play"
        onClicked: hardwareController.injectCommand("BTN_PLAY")
      }
      Button {
        text: "Mock: Next"
        onClicked: hardwareController.injectCommand("BTN_NEXT")
      }
      Button {
        text: "Mock: Vol 50"
        onClicked: hardwareController.injectCommand("VOL:50")
      }
    }
  }
}
