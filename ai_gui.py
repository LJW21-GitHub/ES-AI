import sys
import pip
from ai import treatment

try:
	import PyQt6.QtGui as qtg
	import PyQt6.QtCore as qtc
	import PyQt6.QtWidgets as qtw
	print("PyQt6 loaded")
except ImportError:
	try:
		import PyQt5.QtGui as qtg
		import PyQt5.QtCore as qtc
		import PyQt5.QtWidgets as qtw
		print("PyQt5 loaded")
	except ImportError:
		try:
			import PySide6.QtGui as qtg
			import PySide6.QtCore as qtc
			import PySide6.QtWidgets as qtw
			print("PySide6 loaded")
		except ImportError:
			try:
				import PySide2.QtGui as qtg
				import PySide2.QtCore as qtc
				import PySide2.QtWidgets as qtw
				print("PySide2 loaded")
			except ImportError:
				print("no Qt python libraries on your device ! trying to install one...")
				pip.main(['install', "PyQt6"])
				try:
					import PyQt6.QtGui as qtg
					import PyQt6.QtCore as qtc
					import PyQt6.QtWidgets as qtw
					print("PyQt6 loaded")
				except ImportError:
					pip.main(['install', "PyQt5"])
					try:
						import PyQt5.QtGui as qtg
						import PyQt5.QtCore as qtc
						import PyQt5.QtWidgets as qtw
						print("PyQt5 loaded")
					except ImportError:
						pip.main(['install', "PySide6"])
						try:
							import PySide6.QtGui as qtg
							import PySide6.QtCore as qtc
							import PySide6.QtWidgets as qtw
							print("PyQt6 loaded")
						except ImportError:
							print("""Error : Unable to load or install graphical interface library,
switching to command line interface.
Please make sure that pip is installed,
and try to import PyQt6, PyQt5, PySide2 or PySide6 by yourself.""")
							input("Press enter to continue in CLI")
							import ai_cli
							sys.exit()


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
		textLabel = qtw.QLabel(
			"Bonjour ! Comment puis-je vous aidez sur le tableau périodique ?")
		textLabel.setStyleSheet("background: #32373c")
		textLabel.setWordWrap(True)
		self.avatar = qtg.QPixmap("assets/avatar.png")
		self.avatar = self.avatar.scaled(qtc.QSize(25, 25))
		self.ai = qtg.QPixmap("assets/ai.png")
		self.ai = self.ai.scaled(qtc.QSize(25, 25))
		imageLabel.setPixmap(self.ai)
		textLabel.setMinimumHeight(35)
		textLabel.setMargin(10)

		self.convGrid.addWidget(
			imageLabel, 0, 0, qtc.Qt.AlignmentFlag.AlignCenter)
		self.convGrid.addWidget(
			textLabel, 0, 1, qtc.Qt.AlignmentFlag.AlignVCenter)
		self.convGrid.setColumnMinimumWidth(0, 0)
		self.convGrid.setVerticalSpacing(0)

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

	def add_image_label(self, x, y):
		x.setMinimumHeight(35)
		x.setMargin(10)
		x.setWordWrap(True)
		self.convGrid.addWidget(
			x,
			self.convGrid.rowCount(),
			1,
			qtc.Qt.AlignmentFlag.AlignVCenter
		)
		self.convGrid.addWidget(
			y,
			self.convGrid.rowCount() - 1,
			0,
			qtc.Qt.AlignmentFlag.AlignCenter
		)

	def add_a_row(self):
		self.convGrid.setRowStretch(self.convGrid.rowCount() - 1, 0)
		inputLabel = qtw.QLabel(self.input.text())
		avatarLabel = qtw.QLabel()
		avatarLabel.setPixmap(self.avatar)
		self.add_image_label(inputLabel, avatarLabel)
		answer = treatment(self.input.text())
		answerLabel = qtw.QLabel(answer)
		answerLabel.setStyleSheet("background: #32373c")
		aiLabel = qtw.QLabel()
		aiLabel.setPixmap(self.ai)
		self.add_image_label(answerLabel, aiLabel)
		self.convGrid.setRowStretch(self.convGrid.rowCount() + 1, 1)
		self.input.clear()


app = qtw.QApplication(sys.argv)

window = MainWindow()

window.show()

app.exec()
