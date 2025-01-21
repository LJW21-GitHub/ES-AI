#!/usr/bin/python3
elementsDictionary = {
	"helium": ["Hélium", 20.3, 3],
	"oxygene": ["Oxygène", 50, 4],
	"carbone": ["Carbone", 30, 5]
}

variablesDictionary = {
	"masse molaire": ["g/mol", 1],
	"numero atomique": ["", 2]
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
			print(f"wordAndNext = {wordAndNext}")
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
					f"la {variable} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elementsDictionary[element][0]} est "
					f"{elementsDictionary[element][variablesDictionary[variable][1]]} "
					+ (variablesDictionary[variable][0])
				)
			for value in question["values"]:
				diff = []
				for i, x in elementsDictionary.items():
					diff.append(
						abs(elementsDictionary[i][variablesDictionary[variable][1]] - value)
						+ "/"
						+ x
					)
				if 0 in diff:
					print(
						f"l'élément avec une {variable} de {value} est le "
						+ diff[diff.index(0)][diff.index(0).index("/")]
					)
