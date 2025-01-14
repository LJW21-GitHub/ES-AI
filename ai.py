#!/usr/bin/python3
class elementClass:
	def __init__(self, name, masmol):
		self.name = name
		self.masmol = masmol


helium = elementClass("Hélium", 23)
oxygene = elementClass("Oxygène", 50)

elementsDictionary = ["helium", "oxygene"]
instancesDictionary = [helium, oxygene]
variablesDictionary = ["masse molaire"]


def normalize(x):
	x = x.lower()
	x = x.replace("é", "e")
	x = x.replace("è", "e")
	return x

print("Nyaah~ uwu")


while True:
	elementsInQues = []
	variablesInQues = []
	valuesInQues = []
	question = input("")
	question = normalize(question)
	for i, c in enumerate(question):
		if (
			c == ","
			and ((question[i-1]).isnumeric() if i-1 > -1)
			and ((question[i+1]).isnumeric() if i < len(question))
		)
			question = question.replace(x, ".")
	
	splitQues = question.split(" ")
	for i, word in enumerate(splitQues):
		if word in elementsDictionary:
			elementsInQues.append(word)
		if any((c in "0123456789.") for c in word):
			valuesInQues.append(word)
		if ("".join(word, (splitQues[i+1] if i < len(splitQues)) in variablesDictionary:
			variablesInQues.append(variablesDictionary[i])
	print(elementsInQues)
	print(variablesInQues)
	print(valuesInQues)
	print(question)
	print(splitQues)
	if len(elementsInQues) > 0 and len(variablesInQues) > 0 and len(valuesInQues) == 0:
		for variable in variablesInQues:
			if variable == "masse molaire":
				variable = "masmol"
			for element in elementsInQues:
				print(f"la {variable} de {getattr(element, 'name', 'elementNameError')} est {getattr(element, variable, 'variableValueError')}")
			for value in valuesInQues:
				print(f"l'élément avec une {variable} de {value} est {''.join(x.name for x in instancesDictionary if x.variable == value)}")
