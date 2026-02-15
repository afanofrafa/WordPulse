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
                Material.background: vm.isRunning ? (vm.isPaused ? Material.Shade700 : Material.Orange) : Material.Grey
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
                color: Material.color(Material.Teal)

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
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 12

                    Repeater {
                        model: vm.topWordsModel

                        delegate: ColumnLayout { // Используем ColumnLayout для бара
                            Layout.alignment: Qt.AlignBottom
                            Layout.fillWidth: true
                            spacing: 8

                            property real maxCount: vm.topWordsModel.maxCount

                            // БАР
                            Rectangle {
                                Layout.fillWidth: true
                                // Важно: задаем preferredHeight, чтобы бар рос снизу вверх
                                Layout.preferredHeight: maxCount > 0 ? (model.count / maxCount) * 280 : 0
                                color: Material.color(Material.Green)
                                radius: 8

                                Behavior on Layout.preferredHeight {
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

                            // Подпись слова
                            Text {
                                text: word
                                font.pixelSize: 11
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideMiddle
                                color: "#333"
                            }

                            // Счётчик
                            Text {
                                text: model.count
                                font.pixelSize: 10
                                font.bold: true
                                color: Material.color(Material.Green, Material.Shade700)
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }
        }
    }
    // 1. Универсальный Диалог
    Dialog {
        id: infoDialog
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        standardButtons: Dialog.Ok

        property int msgType: 0 // Храним тип текущего сообщения

        // Заголовок зависит от типа
        title: {
            switch(msgType) {
                case WordPulseViewModel.MsgError: return "Ошибка";
                case WordPulseViewModel.MsgWarning: return "Внимание";
                case WordPulseViewModel.MsgInfo: return "Информация";
                default: return "Сообщение";
            }
        }

        // Иконка или цвет текста
        Label {
            text: infoDialog.msgText // Текст сообщения (см. ниже)
            width: 300
            wrapMode: Text.WordWrap

            // Цвет зависит от типа
            color: {
                switch(infoDialog.msgType) {
                    case WordPulseViewModel.MsgError: return Material.color(Material.Red);
                    case WordPulseViewModel.MsgWarning: return Material.color(Material.Orange, Material.Shade800);
                    case WordPulseViewModel.MsgInfo: return Material.color(Material.Blue);
                    default: return "black";
                }
            }
            font.bold: infoDialog.msgType === WordPulseViewModel.MsgError // Ошибки жирным
        }

        // Временное свойство для текста, чтобы Label его видел
        property string msgText: ""
    }

    // 2. Связь (Ловим сигнал)
    Connections {
        target: vm

        // Обработчик: on + SystemMessage (название сигнала с большой буквы)
        function onSystemMessage(type, text) {
            infoDialog.msgType = type; // Запоминаем тип (0, 1 или 2)
            infoDialog.msgText = text; // Запоминаем текст
            infoDialog.open();         // Показываем
        }
    }
}
