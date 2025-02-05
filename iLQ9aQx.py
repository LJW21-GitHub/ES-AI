import os
import filecmp
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLabel, QPushButton, QFileDialog, QLineEdit, QTextEdit, QCheckBox, QTabWidget, QFrame, QColorDialog, QProgressBar
)
from PyQt6.QtGui import QTextCursor, QFont
from PyQt6.QtCore import Qt, QTimer

class FileComparatorApp(QWidget):
    def __init__(self):
        super().__init__()
        self.init_ui()

    def init_ui(self):
        """
        Initialize the user interface.
        """
        self.setWindowTitle("File Comparator")
        self.resize(1000, 800)

        layout = QVBoxLayout()

        # Tabs
        self.tabs = QTabWidget()
        self.main_tab = QWidget()
        self.options_tab = QWidget()
        self.tabs.addTab(self.main_tab, "Main")
        self.tabs.addTab(self.options_tab, "Options")
        layout.addWidget(self.tabs)

        # Main Tab Content
        main_layout = QVBoxLayout()

        # Input for Folder 1
        self.folder1_label = QLabel("Folder 1:")
        main_layout.addWidget(self.folder1_label)
        self.folder1_input = QLineEdit()
        main_layout.addWidget(self.folder1_input)
        self.folder1_button = QPushButton("Select Folder 1")
        self.folder1_button.clicked.connect(lambda: self.select_folder(self.folder1_input))
        main_layout.addWidget(self.folder1_button)

        # Input for Folder 2
        self.folder2_label = QLabel("Folder 2:")
        main_layout.addWidget(self.folder2_label)
        self.folder2_input = QLineEdit()
        main_layout.addWidget(self.folder2_input)
        self.folder2_button = QPushButton("Select Folder 2")
        self.folder2_button.clicked.connect(lambda: self.select_folder(self.folder2_input))
        main_layout.addWidget(self.folder2_button)

        # Compare Button
        self.compare_button = QPushButton("Start Comparison")
        self.compare_button.clicked.connect(self.compare_files)
        main_layout.addWidget(self.compare_button)

        # Debug Console (Styled as Terminal)
        self.debug_console = QTextEdit()
        self.debug_console.setReadOnly(True)
        self.debug_console.setFont(QFont("Courier", 10))
        self.debug_console.setStyleSheet("background-color: black; color: white;")
        main_layout.addWidget(self.debug_console)

        # Progress Bar
        self.progress_bar = QProgressBar()
        self.progress_bar.setValue(0)
        main_layout.addWidget(self.progress_bar)

        self.main_tab.setLayout(main_layout)

        # Options Tab Content
        options_layout = QVBoxLayout()

        # Frame for Filters
        filter_frame = QFrame()
        filter_layout = QVBoxLayout()
        self.extension_input = QLineEdit()
        self.extension_input.setPlaceholderText("File extension to compare (default: txt)")
        self.extension_input.setText("txt")
        filter_layout.addWidget(QLabel("File Extension:"))
        filter_layout.addWidget(self.extension_input)
        filter_frame.setLayout(filter_layout)
        filter_frame.setFrameShape(QFrame.Shape.Box)
        options_layout.addWidget(filter_frame)

        # Frame for Display Options
        display_frame = QFrame()
        display_layout = QVBoxLayout()
        self.show_differences_only = QCheckBox("Show Differences Only")
        self.ignore_absent = QCheckBox("Ignore Absent Files")
        display_layout.addWidget(self.show_differences_only)
        display_layout.addWidget(self.ignore_absent)
        display_frame.setLayout(display_layout)
        display_frame.setFrameShape(QFrame.Shape.Box)
        options_layout.addWidget(display_frame)

        # Frame for Colors
        color_frame = QFrame()
        color_layout = QVBoxLayout()
        color_layout.addWidget(QLabel("Color Coding:"))
        self.color_diff = QPushButton("Differences (Red)")
        self.color_diff.clicked.connect(lambda: self.select_color(self.color_diff))
        self.color_absent = QPushButton("Absent (Orange)")
        self.color_absent.clicked.connect(lambda: self.select_color(self.color_absent))
        self.color_same = QPushButton("Identical (Green)")
        self.color_same.clicked.connect(lambda: self.select_color(self.color_same))
        self.color_info = QPushButton("Info (Blue)")
        self.color_info.clicked.connect(lambda: self.select_color(self.color_info))
        color_layout.addWidget(self.color_diff)
        color_layout.addWidget(self.color_absent)
        color_layout.addWidget(self.color_same)
        color_layout.addWidget(self.color_info)
        color_frame.setLayout(color_layout)
        color_frame.setFrameShape(QFrame.Shape.Box)
        options_layout.addWidget(color_frame)

        # Explanation of Colors
        explanation_label = QLabel("Color Coding Explanation:\nRed: Differences\nOrange: File Absent\nGreen: Identical Files\nBlue: Information")
        options_layout.addWidget(explanation_label)

        self.options_tab.setLayout(options_layout)

        self.setLayout(layout)

        # Default Colors
        self.colors = {
            "diff": Qt.GlobalColor.red,
            "absent": Qt.GlobalColor.darkYellow,
            "same": Qt.GlobalColor.green,
            "info": Qt.GlobalColor.blue
        }

        # Welcome Message
        self.append_colored_text("> Welcome to File Comparator!", self.colors["info"])
        self.append_colored_text("> Compare files efficiently and easily.", self.colors["info"])
        self.append_colored_text("> Thank you for using this application!", self.colors["info"])

    def select_color(self, button):
        """
        Opens a color picker dialog and assigns the selected color to the corresponding category.
        
        Args:
            button (QPushButton): The button associated with the color category.
        """
        color = QColorDialog.getColor()
        if color.isValid():
            if button == self.color_diff:
                self.colors["diff"] = color
            elif button == self.color_absent:
                self.colors["absent"] = color
            elif button == self.color_same:
                self.colors["same"] = color
            elif button == self.color_info:
                self.colors["info"] = color

    def select_folder(self, input_field):
        """
        Opens a folder selection dialog and sets the selected folder path in the input field.

        Args:
            input_field (QLineEdit): The input field to update with the folder path.
        """
        folder = QFileDialog.getExistingDirectory(self, "Select a folder")
        if folder:
            input_field.setText(folder)

    def get_files(self, folder, extension):
        """
        Retrieves a list of files with the given extension from the specified folder.

        Args:
            folder (str): The folder to search in.
            extension (str): The file extension to filter by.

        Returns:
            list: A list of file paths matching the extension.
        """
        files = []
        for root, _, files_list in os.walk(folder):
            for file in files_list:
                if file.endswith(f".{extension}"):
                    files.append(os.path.join(root, file))
        return files

    def append_colored_text(self, text, color):
        """
        Appends colored text to the debug console, simulating a terminal-like appearance.

        Args:
            text (str): The text to display.
            color (Qt.GlobalColor): The color of the text.
        """
        self.debug_console.setTextColor(color)
        for char in text:
            self.debug_console.insertPlainText(char)
            QApplication.processEvents()  # Simulate typewriter effect
        self.debug_console.insertPlainText("\n")
        self.debug_console.setTextColor(Qt.GlobalColor.white)

    def compare_files(self):
        """
        Compares files between two selected folders based on the specified extension.
        Displays the comparison results in the debug console.
        """
        folder1 = self.folder1_input.text()
        folder2 = self.folder2_input.text()
        extension = self.extension_input.text().strip()

        if not folder1 or not folder2 or not extension:
            self.append_colored_text("> Please select both folders and specify an extension.", self.colors["diff"])
            return

        self.debug_console.clear()
        self.append_colored_text(f"> Folder 1: {folder1}", self.colors["info"])
        self.append_colored_text(f"> Folder 2: {folder2}", self.colors["info"])

        files1 = self.get_files(folder1, extension)
        files2 = self.get_files(folder2, extension)

        relative_files1 = {os.path.relpath(f, folder1): f for f in files1}
        relative_files2 = {os.path.relpath(f, folder2): f for f in files2}

        all_files = set(relative_files1.keys()).union(set(relative_files2.keys()))

        self.progress_bar.setMaximum(len(all_files))
        self.progress_bar.setValue(0)

        for i, file in enumerate(all_files, start=1):
            self.progress_bar.setValue(i)

            file1_path = relative_files1.get(file)
            file2_path = relative_files2.get(file)

            if file1_path and file2_path:
                # Both files exist, compare them
                if filecmp.cmp(file1_path, file2_path, shallow=False):
                    if not self.show_differences_only.isChecked():
                        self.append_colored_text(f"> {file} is identical", self.colors["same"])
                else:
                    self.append_colored_text(f"> {file} differs", self.colors["diff"])
            elif file1_path:
                # File is only in Folder 1
                if not self.ignore_absent.isChecked():
                    self.append_colored_text(f"> {file} is only in Folder 1", self.colors["absent"])
            elif file2_path:
                # File is only in Folder 2
                if not self.ignore_absent.isChecked():
                    self.append_colored_text(f"> {file} is only in Folder 2", self.colors["absent"])

        self.append_colored_text("> Comparison complete!", self.colors["info"])
        self.progress_bar.setValue(len(all_files))


if __name__ == "__main__":
    import sys

    app = QApplication(sys.argv)
    comparator = FileComparatorApp()
    comparator.show()
    sys.exit(app.exec())

