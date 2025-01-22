#!/usr/bin/python3
elementsDictionary = {
	"helium": ["Hélium", 20.3, 3],
	"oxygene": ["Oxygène", 50, 4],
	"carbone": ["Carbone", 30, 5]
}

variablesDictionary = {
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
			and ((x[i-1]) in "0123456789" if i > 0 else False)
			and ((x[i+1]) in "0123456789" if i < len(question)-1 else False)
		):
			x = x.replace(c, ".")
	splitQues = x.split(" ")
	for i, word in enumerate(splitQues):
		if word in elementsDictionary:
			analysis["elements"].append(word)
		try:
			analysis["values"].append(int(word))
		except ValueError:
			try:
				analysis["values"].append(float(word))
			except ValueError:
				pass
		if word in variablesDictionary:
			analysis["variables"].append(word)
		else:
			wordAndNext = \
				"".join(list(word + " " + splitQues[i+1] if i < len(splitQues)-1 else ""))
			if wordAndNext in variablesDictionary:
				analysis["variables"].append(wordAndNext)
	if len(analysis["variables"]) and (
		(len(analysis["elements"]) == 0)
		!= (len(analysis["values"]) == 0)
	):
		analysis["typeOfSent"] = "question"
	return analysis


print("Nyaah~ uwu")


while True:
	question = input("")
	question = analyze(question)
	print(question)
	if question["typeOfSent"] == "question":
		for variable in question["variables"]:
			for element in question["elements"]:
				print(
					("la " if variablesDictionary[variable][3] == "f" else "le ")
					+ f"{variablesDictionary[variable][0]} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elementsDictionary[element][0]} est "
					f"{elementsDictionary[element][variablesDictionary[variable][2]]} "
					+ (variablesDictionary[variable][1])
				)
			for value in question["values"]:
				diff = {}
				for i, x in elementsDictionary.items():
					diff[str(i)] = abs(
						x[variablesDictionary[variable][2]]
						- value
					)
				print(diff)
				if 0 in diff.values():
					print(
						"l'élément avec une "
						+ variable
						+ " de "
						+ value
						+ " est le "
						+ (
							"".join(list(i)) for i, x in diff.items() if min(diff.values()) == x
						)
					)
