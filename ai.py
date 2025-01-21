#!/usr/bin/python3
elementsDictionary = {
	"helium": ["Hélium", 23],
	"oxygene": ["Oxygène", 50]
}

variablesDictionary = {
	"masse molaire": ["masmol", "g/mol", 1]
}


def normalize(x):
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
	return x


print("Nyaah~ uwu")


while True:
	elementsInQues = []
	variablesInQues = []
	valuesInQues = []
	question = input("")
	question = normalize(question)
	splitQues = question.split(" ")
	for i, word in enumerate(splitQues):
		if word in elementsDictionary:
			elementsInQues.append(word)
		try:
			tryFloat = float(word)
			valuesInQues.append(tryFloat)
		except ValueError:
			pass
		if word in variablesDictionary:
			variablesInQues.append(word)
		else:
			wordAndNext = \
				"".join(list(word + " " + splitQues[i+1] if i < len(splitQues)-1 else ""))
			print(f"wordAndNext = {wordAndNext}")
			if wordAndNext in variablesDictionary:
				variablesInQues.append(wordAndNext)
	print(elementsInQues)
	print(variablesInQues)
	print(valuesInQues)
	print(question)
	print(splitQues)
	for variable in variablesInQues:
		for variable in variablesInQues:
			for element in elementsInQues:
				print(
					f"la {variable} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elementsDictionary[element][0]} est "
					f"{elementsDictionary[element][variablesDictionary[variable][2]]} "
					f"{variablesDictionary[variable][1]}"
				)
			for value in valuesInQues:
				print(
					f"l'élément avec une {variable} de {value} est de"
					f"{
						elementsDictionary[x][0]
						if elementsDictionary[x][variablesDictionary[variable][2]] == value
						for x in elementDictionary
						else 'meow'
					}"
					f"l'élément avec une {variable} de {value} est le"
					+ (
						elementsDictionary[x][0]
						for x in elementDictionary
						if elementsDictionary[x][variablesDictionary[variable][2]] == value
						else 'meow'
					)
				)
