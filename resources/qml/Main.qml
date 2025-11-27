import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import WordPulse 1.0

ApplicationWindow {
    id: window
    width: 1100; height: 750
    visible: true
    title: "WordPulse"

    WordPulseViewModel { id: vm }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20

        // КНОПКИ
        RowLayout {
            spacing: 15
            Button {
                text: "Открыть"
                enabled: !vm.isRunning
                onClicked: vm.openFile()
                Material.background: Material.Blue
            }
            Button {
                text: vm.isRunning ? (vm.isPaused ? "Продолжить" : "Пауза") : "Старт"
                onClicked: {
                    if (!vm.isRunning)
                        vm.start()
                    else {
                        if (vm.isPaused)
                            vm.resume()
                        else
                            vm.pause()
                    }
                }
                Material.background: vm.isRunning ? (vm.isPaused ? Material.Green : Material.Orange) : Material.Grey
            }
            Button {
                text: "Отмена"
                enabled: true === vm.isRunning
                onClicked: vm.cancel()
                Material.background: Material.Red
            }
        }

        // ПРОГРЕСС
        Rectangle {
            Layout.fillWidth: true
            height: 40
            visible: true//vm.isRunning
            radius: 20
            color: "#e0e0e0"

            Rectangle {
                width: parent.width * (vm.progress / 100)
                height: parent.height
                radius: 20
                color: Material.color(Material.Green)

                Behavior on width {
                        enabled: vm.progress > 0 && vm.isRunning  // ← ВКЛЮЧАЕТ АНИМАЦИЮ ТОЛЬКО ПРИ РОСТЕ
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }
            }

            Label {
                anchors.centerIn: parent
                text: vm.progress + "%"
                color: vm.progress > 50 ? "white" : "#333"
                font.bold: true
            }
            border.color: "#ccc"
            border.width: 1
        }

        // ГИСТОГРАММА
        // ГИСТОГРАММА
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 420
            color: "white"
            radius: 16
            border.color: "#ddd"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 25
                spacing: 15

                Text {
                    text: "Топ-" + vm.topWordsCount + " слов"
                    font.pixelSize: 24
                    font.bold: true
                }

                // ← КЛЮЧЕВОЙ ФИКС: GridLayout вместо RowLayout + Repeater
                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: vm.topWordsCount > 0 ? vm.topWordsCount : 1
                    columnSpacing: 12
                    rowSpacing: 20

                    Repeater {
                        model: vm.topWordsModel

                        delegate: Column {
                            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            spacing: 8

                            property real maxCount: vm.topWordsModel.maxCount

                            // БАР — теперь ширина всегда одинаковая!
                            Rectangle {
                                width: parent.width - 20  // отступы по бокам
                                height: maxCount > 0 ? (model.count / maxCount) * 280 : 0
                                color: Material.color(Material.Green)
                                radius: 8
                                anchors.horizontalCenter: parent.horizontalCenter

                                Behavior on height {
                                    NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
                                }

                                ToolTip {
                                    visible: mouseArea.containsMouse
                                    text: (word.length > 10 ? word.substring(0, 10) + "..." : word) + ": " + count
                                    delay: 500
                                }

                                MouseArea {
                                    id: mouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                }
                            }

                            // Подпись слова — с переносом и обрезкой
                            Text {
                                text: word//(word.length > 10 ? word.substring(0, 0) + "..." : word)
                                font.pixelSize: 11
                                width: parent.width - 10
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideMiddle
                                color: "#333"
                            }

                            // Счётчик (опционально, красиво)
                            Text {
                                text: model.count
                                font.pixelSize: 10
                                font.bold: true
                                color: Material.color(Material.Green, Material.Shade700)
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
