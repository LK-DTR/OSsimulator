import QtQuick
import QtQuick.Controls
import QtQuick.Controls.FluentWinUI3
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    width: 500
    height: 700
    visible: true
    title: "Simulador de SO - Eng. de Computação"

    FileDialog {
        id: fileDialog
        title: "Selecione o arquivo de processos"
        nameFilters: ["Arquivos CSV (*.csv)", "Todos os Arquivos (*)"]
        onAccepted: csvPathField.text = selectedFile.toString().replace("file:///", "")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 15

        Label {
            text: "Configuração da Simulação"
            font.bold: true
            font.pixelSize: 18
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 15
        }

        // --- 1. ENTRADA DE DADOS ---
        Label {
            text: "Entrada de Dados"
        }

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
        Label {
            text: "Gerenciamento de Memória (MB)"
        }

        RowLayout {
            spacing: 15
            Layout.fillWidth: true

            TextField {
                id: memFisica
                placeholderText: "Física (ex: 1024)"
                // text: "1024"
                Layout.fillWidth: true
            }

            TextField {
                id: memVirtual
                placeholderText: "Virtual (ex: 4096)"
                // text: "4096"
                Layout.fillWidth: true
            }
        }

        // --- 3. POLÍTICAS ---
        Label {
            text: "Políticas do Núcleo"
        }

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
        Item {
            Layout.fillHeight: true
        }

        Button {
            text: "Iniciar Simulação"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            highlighted: true
            onClicked: {
                backend.iniciarSimulacao(csvPathField.text, parseInt(memFisica.text), parseInt(memVirtual.text), comboEscalonamento.currentIndex, parseInt(inputQuantum.text), comboPolitica.currentIndex);
            }
        }
    }
}
