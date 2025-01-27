#!/usr/bin/python3
import sys
import PyQt6.QtGui as qtg
import PyQt6.QtCore as qtc
import PyQt6.QtWidgets as qtw


class MainWindow(qtw.QMainWindow):
    def __init__(self):
        super().__init__()

        widget = qtw.QWidget()
        overallLayout = qtw.QVBoxLayout(widget)
        scroll = qtw.QScrollArea()
        convGridWidget = qtw.QWidget()
        self.convGrid = qtw.QGridLayout()

        convGridWidget.setLayout(self.convGrid)
        convGridWidget.setSizePolicy(qtw.QSizePolicy.Policy.Ignored, qtw.QSizePolicy.Policy.Ignored)

        imagelabel = qtw.QLabel()
        textLabel = qtw.QLabel("test")
        image = qtg.QPixmap("avatar.png")
        image = image.scaled(qtc.QSize(75, 75))
        imagelabel.setPixmap(image)

        self.convGrid.addWidget(
            imagelabel, 0, 0, qtc.Qt.AlignmentFlag.AlignCenter)
        self.convGrid.addWidget(
            textLabel, 0, 1, qtc.Qt.AlignmentFlag.AlignVCenter)
        self.convGrid.setColumnMinimumWidth(0, 125)
        self.convGrid.setColumnStretch(1, 1)
        scroll.setWidget(convGridWidget)
        scroll.setSizePolicy(qtw.QSizePolicy.Policy.Ignored, qtw.QSizePolicy.Policy.Ignored)

        self.input = qtw.QLineEdit()
        self.input.setMaximumHeight(100)

        overallLayout.addWidget(scroll)
        overallLayout.addWidget(self.input)

        self.setCentralWidget(widget)

        self.input.returnPressed.connect(self.add_a_row)

    def add_a_row(self):
        self.convGrid.addWidget(
            qtw.QLabel(self.input.text()),
            self.convGrid.rowCount() + 1,
            1,
            qtc.Qt.AlignmentFlag.AlignVCenter
        )


app = qtw.QApplication(sys.argv)

window = MainWindow()

window.show()

app.exec()
