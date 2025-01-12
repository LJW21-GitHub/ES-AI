#!/usr/bin/python3
class elementClass:
	def __init__(self, name, masmol):
		self.name = name
		self.masmol = masmol

helium = elementClass("Hélium", 23)
oxygene = elementClass("Oxygène", 50)

elementsDictionary = ["helium", "oxygene"]
instancesDictionary = [helium, oxygene]
variablesBeginning = ["masse"]
variablesEnd = ["molaire"]
variablesDictionary = ["masse molaire"]

def normalize(x):
	x = x.lower()
	x = x.replace("é", "e")
	x = x.replace("è", "e")
	return x

print("Nyaah~~ uwu")

while True:
	elementsInQues = []
	variablesInQues = []
	valuesInQues = []
	question = input("")
	question = normalize(question)
	splitQues = question.split(" ")
	for index, word in enumerate(splitQues):
		try:
			if elementsDictionary.index(word) > -1:
				elementsInQues.append(word)
		except ValueError:
			pass
			if word.isnumeric():
				valuesInQues.append(word)
		try:
			if variablesBeginning.index(word) > -1:
				if variablesEnd.index(splitQues[index+1]) == variablesBeginning.index(word):
					variablesInQues.append(variablesDictionary[index])
		except ValueError:
			pass
	print(elementsInQues)
	print(variablesInQues)
	print(valuesInQues)
	print(question)
	print(splitQues)
	if len(elementsInQues) > 0 and len(variablesInQues) > 0 and len(valuesInQues) == 0:
		for variable in variablesInQues:
			for element in elementsInQues:
				print(f"la {variable} de {getattr({element}, 'name', 'elementNameError')} est {getattr({element}, variable, 'variableValueError')}")
			for value in valuesInQues:
				print(f"l'élément avec une {variable} de {value} est {''.join(x.name for x in instancesDictionary if x.variable == value)}")
