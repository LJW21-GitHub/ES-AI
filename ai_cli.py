elemDict = {
	"hydrogene": ["Hydrogène", 1.008, 1],
	"helium": ["Hélium", 4.003, 2],
	"lithium": ["Lithhium", 6.94, 3],
	"beryllium": ["Béryllium", 9.012, 4],
	"bore": ["Bore", 10.81, 5],
	"carbone": ["Carbone", 12.01, 6],
	"azote": ["Azote", 14.01, 7],
	"oxygene": ["Oxygène", 16, 8],
	"fluor": ["Fluor", 19, 9],
	"neon": ["Néon", 20.18, 10],
	"sodium": ["Sodium", 22.99, 11],
	"magnesium": ["Magnésium", 24.31, 12],
	"aluminium": ["Aluminium", 26.98, 13],
	"silicium": ["Silicium", 28.09, 14],
	"phosphore": ["Phosphore", 30.97, 15],
	"soufre": ["Soufre", 32.06, 16],
	"chlore": ["Chlore", 35.45, 17],
	"argon": ["Argon", 39.95, 18],
	"potassium": ["Potassium", 39.1, 19],
	"calcium": ["Calcium", 40.08, 20],
	"scandium": ["Scandium", 44.96, 21],
	"titane": ["Titane", 47.87, 22],
	"vanadium": ["Vanadium", 50.94, 23],
	"chrome": ["Chrome", 52, 24],
	"manganese": ["Manganèse", 54.94, 25],
	"fer": ["Fer", 55.85, 26],
	"cobalt": ["Cobalt", 58.93, 27],
	"nickel": ["Nickel", 58.69, 28],
	"cuivre": ["Cuivre", 63.55, 29],
	"zinc": ["Zinc", 65.38, 30],
	"gallium": ["Gallium", 69.72, 31],
	"germanium": ["Germanium", 72.63, 32],
	"arsenic": ["Arsenic", 74.92, 33],
	"selenium": ["Sélénium", 78.97, 34],
	"brome": ["Brome", 79.9, 35]
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


def treatment():
	question = input("")
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

print("Nyaah~ uwu")
while True:
	print(treatment())
