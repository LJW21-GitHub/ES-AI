#!/usr/bin/python3
import sys
import PyQt6.QtGui as qtg
import PyQt6.QtCore as qtc
import PyQt6.QtWidgets as qtw


class MainWindow(qtw.QMainWindow):
    def __init__(self):
        super().__init__()

        self.widget = qtw.QWidget()
        overallLayout = qtw.QVBoxLayout(self.widget)
        scroll = qtw.QScrollArea()
        convGridWidget = qtw.QWidget()
        self.convGrid = qtw.QGridLayout(convGridWidget)

        convGridWidget.setSizePolicy(
            qtw.QSizePolicy.Policy.Expanding,
            qtw.QSizePolicy.Policy.Expanding)

        imageLabel = qtw.QLabel()
        textLabel = qtw.QLabel("test")
        image = qtg.QPixmap("avatar.png")
        image = image.scaled(qtc.QSize(25, 25))
        imageLabel.setPixmap(image)
        textLabel.setMaximumHeight(75)

        self.convGrid.addWidget(
            imageLabel, 0, 0, qtc.Qt.AlignmentFlag.AlignCenter)
        self.convGrid.addWidget(
            textLabel, 0, 1, qtc.Qt.AlignmentFlag.AlignVCenter)
        self.convGrid.setColumnMinimumWidth(0, 50)

        self.convGrid.setColumnStretch(1, 1)
        self.convGrid.setRowStretch(0, 0)
        scroll.setWidget(convGridWidget)
        scroll.setWidgetResizable(True)
        scroll.setSizePolicy(
            qtw.QSizePolicy.Policy.Expanding, qtw.QSizePolicy.Policy.Expanding)

        self.input = qtw.QLineEdit()
        self.input.setMaximumHeight(50)

        self.convGrid.setRowStretch(self.convGrid.rowCount(), 1)

        overallLayout.addWidget(scroll)
        overallLayout.addWidget(self.input)

        self.setCentralWidget(self.widget)

        self.input.returnPressed.connect(self.add_a_row)

    def add_a_row(self):
        self.convGrid.setRowStretch(self.convGrid.rowCount() - 1, 0)
        self.convGrid.addWidget(
            qtw.QLabel(self.input.text()),
            self.convGrid.rowCount() - 1,
            1,
            qtc.Qt.AlignmentFlag.AlignVCenter
        )
        self.convGrid.setRowMinimumHeight(self.convGrid.rowCount() - 1, 25)
        self.convGrid.setRowStretch(self.convGrid.rowCount(), 1)
        self.input.clear()


app = qtw.QApplication(sys.argv)

window = MainWindow()

window.show()

app.exec()
