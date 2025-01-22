#!/usr/bin/python3
import sys
import PyQt6.QtCore as qtc
import PyQt6.QtWidgets as qtw


class MainWindow(qtw.QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")
        button = qtw.QPushButton("Press Me!")
        button.setMinimumSize(qtc.QSize(100, 50))
        button.setMaximumSize(qtc.QSize(300, 150))

        # Set the central widget of the Window.
        self.setCentralWidget(button)


app = qtw.QApplication(sys.argv)

window = MainWindow()

window.show()

app.exec()
