#!/bin/python3.12
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
			and ((x[i + 1]) in "0123456789" if i < len(question) - 1 else False)
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


while True:
	question = input("")
	question = analyze(question)
	print(question)
	if question["typeOfSent"] == "question":
		for variable in question["variables"]:
			for element in question["elements"]:
				print(
					("la " if varDict[variable][3] == "f" else "le ")
					+ f"{varDict[variable][0]} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elemDict[element][0]} est "
					f"{elemDict[element][varDict[variable][2]]} "
					+ (varDict[variable][1])
				)
			for value in question["values"]:
				print(value)
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
					print(
						"l'élément avec une "
						+ variable
						+ " de "
						+ str(value)
						+ (" est l'" if ans[0] in "aeiouyh" else " est le ")
						+ elemDict[ans][0]
					)
