#!/usr/bin/python3
import sys
import pip
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
							pass


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


elemDict = {
	"helium": ["Hélium", 20.3, 3],
	"oxygene": ["Oxygène", 50, 4],
	"carbone": ["Carbone", 30, 5]
}

varDict = {
	"masse molaire": ["masse molaire", "g/mol", 1, "f"],
	"numero atomique": ["numéro atomique", "", 2, "m"]
}


def analyze(x):
	analysis = {
		"elements": [],
		"variables": [],
		"values": [],
		"typeOfSent": "",
	}
	x = x.lower()
	x = x.replace("é", "e")
	x = x.replace("è", "e")
	x = x.replace("'", " ")
	for i, c in enumerate(x):
		if (
			c == ","
			and ((x[i - 1]) in "0123456789" if i > 0 else False)
			and ((x[i + 1]) in "0123456789" if i < len(x) - 1 else False)
		):
			x = x.replace(c, ".")
	splitQues = x.split(" ")
	for i, word in enumerate(splitQues):
		if word in elemDict:
			analysis["elements"].append(word)
		try:
			analysis["values"].append(int(word))
		except ValueError:
			try:
				analysis["values"].append(float(word))
			except ValueError:
				pass
		if word in varDict:
			analysis["variables"].append(word)
		else:
			wordAndNext = \
				"".join(list(
					word + " " + splitQues[i + 1]
					if i < len(splitQues) - 1 else "")
				)
			if wordAndNext in varDict:
				analysis["variables"].append(wordAndNext)
	if len(analysis["variables"]) and (
		(len(analysis["elements"]) == 0)
		!= (len(analysis["values"]) == 0)
	):
		analysis["typeOfSent"] = "question"
	return analysis


print("Nyaah~ uwu")


def treatment(question):
	# question = input("")
	print(question)
	question = analyze(question)
	print(question)
	if question["typeOfSent"] == "question":
		for variable in question["variables"]:
			for element in question["elements"]:
				return (
					("la " if varDict[variable][3] == "f" else "le ")
					+ f"{varDict[variable][0]} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elemDict[element][0]} est "
					f"{elemDict[element][varDict[variable][2]]} "
					+ (varDict[variable][1])
				)
			for value in question["values"]:
				diff = {}
				for i, x in elemDict.items():
					diff[str(i)] = abs(
						x[varDict[variable][2]]
						- value
					)
				print(diff)
				if 0 in diff.values():
					ans = "".join(list(
						i for i, x in diff.items() if min(diff.values()) == x))
					return (
						"l'élément avec une "
						+ variable
						+ " de "
						+ str(value)
						+ (" est l'" if ans[0] in "aeiouyh" else " est le ")
						+ elemDict[ans][0]
					)


app = qtw.QApplication(sys.argv)

window = MainWindow()

window.show()

app.exec()
