import QtQuick
import QtQuick.Controls
import QtQuick.Controls.FluentWinUI3
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: janela
    width: 580
    height: 840
    visible: true
    title: "Simulador de SO - Eng. de Computação"

    // Paleta para diferenciar os processos na linha do tempo.
    readonly property var coresProc: ["#4F8DFD", "#FD7E4F", "#3FB984", "#B45FD8", "#E0B23F", "#D85F8A", "#5FC9D8", "#9CCB4F"]
    function corDoPid(pid) { return coresProc[Math.abs(pid) % coresProc.length] }

    FileDialog {
        id: fileDialog
        title: "Selecione o arquivo de processos"
        nameFilters: ["Arquivos CSV (*.csv)", "Todos os Arquivos (*)"]
        onAccepted: csvPathField.text = selectedFile.toString().replace("file:///", "")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 12

        Label {
            text: "Configuração da Simulação"
            font.bold: true
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        // --- 1. ENTRADA DE DADOS ---
        Label { text: "Entrada de Dados" }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                id: csvPathField
                placeholderText: "Caminho do arquivo CSV..."
                Layout.fillWidth: true
                readOnly: true
            }

            Button {
                text: "Procurar"
                onClicked: fileDialog.open()
            }
        }

        // --- 2. MEMÓRIA ---
        Label { text: "Gerenciamento de Memória (MB)" }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            TextField {
                id: memFisica
                placeholderText: "Física (ex: 1024)"
                Layout.fillWidth: true
            }

            TextField {
                id: memVirtual
                placeholderText: "Virtual (ex: 4096)"
                Layout.fillWidth: true
            }
        }

        // --- 3. POLÍTICAS ---
        Label { text: "Políticas do Núcleo" }

        ComboBox {
            id: comboEscalonamento
            Layout.fillWidth: true
            model: ["Round-Robin (RR)", "SJF Preemptivo", "Prioridade Preemptiva"]
        }

        TextField {
            id: inputQuantum
            placeholderText: "Valor do Quantum"
            text: "2"
            visible: comboEscalonamento.currentIndex === 0
            Layout.fillWidth: true
        }

        ComboBox {
            id: comboPolitica
            Layout.fillWidth: true
            model: ["Substituição: FIFO", "Substituição: LRU", "Substituição: Ótimo"]
        }

        // --- 4. BOTÃO INICIAR ---
        Button {
            text: "Iniciar Simulação"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            highlighted: true
            onClicked: {
                backend.iniciarSimulacao(
                    csvPathField.text,
                    parseInt(memFisica.text) || 1024,
                    parseInt(memVirtual.text) || 4096,
                    comboEscalonamento.currentIndex,
                    parseInt(inputQuantum.text) || 1,
                    comboPolitica.currentIndex);
            }
        }

        // --- 5. RELATÓRIOS ---
        Label {
            text: "Linha do Tempo (Gantt)"
            font.bold: true
            visible: backend.gantt.length > 0
        }

        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            visible: backend.gantt.length > 0
            contentWidth: ganttRow.width
            clip: true
            flickableDirection: Flickable.HorizontalFlick

            Row {
                id: ganttRow
                spacing: 1
                Repeater {
                    model: backend.gantt
                    delegate: Column {
                        spacing: 2
                        Rectangle {
                            width: Math.max(28, modelData.duracao * 22)
                            height: 36
                            color: janela.corDoPid(modelData.pid)
                            radius: 4
                            Label {
                                anchors.centerIn: parent
                                text: "P" + modelData.pid
                                color: "white"
                                font.bold: true
                            }
                        }
                        Label {
                            text: modelData.inicio + "→" + modelData.fim
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            clip: true

            TextArea {
                readOnly: true
                wrapMode: TextEdit.NoWrap
                font.family: "monospace"
                text: backend.relatorio.length > 0
                      ? backend.relatorio
                      : "Selecione um CSV e inicie a simulação para ver os resultados."
            }
        }
    }
}
