#!/usr/bin/python3
class elementClass:
	def __init__(self, name, masmol):
		self.name = name
		self.masmol = masmol


helium = elementClass("Hélium", 23)
oxygene = elementClass("Oxygène", 50)

elementsDictionary = {
	"helium": helium,
	"oxygene": oxygene
}

variablesDictionary = {
	"masse molaire": "masmol"
}


def normalize(x):
	x = x.lower()
	x = x.replace("é", "e")
	x = x.replace("è", "e")
	for i, c in enumerate(x):
		if (
			c == ","
			and ((x[i-1]).isnumeric() if i > 0 else False)
			and ((x[i+1]).isnumeric() if i < len(question)-1 else False)
		):
			c = c.replace(",", ".")
	return x


print("Nyaah~ uwu")


while True:
	elementsInQues = []
	variablesInQues = []
	valuesInQues = []
	question = "masse molaire hélium"
	question = normalize(question)
	splitQues = question.split(" ")
	for i, word in enumerate(splitQues):
		if word in elementsDictionary:
			elementsInQues.append(word)
		if any((c in "0123456789.") for c in word):
			valuesInQues.append(word)
		wordAndNext = \
			" ".join(list(word, splitQues[i+1] if i < len(splitQues)-1 else ""))
		if wordAndNext in variablesDictionary:
			variablesInQues.append(variablesDictionary[wordAndNext])
	print(elementsInQues)
	print(variablesInQues)
	print(valuesInQues)
	print(question)
	print(splitQues)
	if (
		len(elementsInQues) > 0
		and len(variablesInQues) > 0
		and len(valuesInQues) == 0
	):
		for variable in variablesInQues:
			variable = variablesDictionary[variable]
			for element in elementsInQues:
				element = elementsDictionary[element]
				print(f"la {variable} de \
					{getattr(element, 'name', 'elementNameError')}\
					est {getattr(element, variable, 'variableValueError')}")
			for value in valuesInQues:
				print(f"l'élément avec une {variable} de {value} est \
					{''.join(x.name for x
						in list(elementsDictionary.values()) if x.variable == value)}")
