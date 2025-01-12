#!/usr/bin/python3
class elementClass:
	def __init__(self, name, masmol):
		self.name = name
		self.masmol = masmol

elementDictionnary = ["helium", "oxygene"]
variablesBeginning = ["masse"]
variablesEnd = ["molaire"]
variablesDictionnary = ["masse molaire"]

helium = elementClass("Hélium", 23)
oxygene = elementClass("Oxygène", 50)

def normalize(name):
	name = name.lower()
	name = name.replace("é", "e")
	name = name.replace("è", "e")

print("Nyaah~~ uwu")

while True:
	question = input("")
	question = normalize(question)
	questionalnum = list([val for val in question if (val.isalnum() or val == " " or val == "." or val == ".")])
	splitQues = question.split(" ")
	for index, word in enumerate(splitQues):
		try:
			if word.index(elementDictionnary):
				elementsInQues.append(word)
		finally:
			pass
		try:
			if word.isnumeric():
				valuesInQues.append(word)
		finally:
			pass
		try:
			if word.index(variablesBeginning):
				nextWord = index+1
				if index+1.index(variablesEnd) = word.index(variablesBeginning):
				#if splitQues([word + 1]) in variablesEnd:
					variablesInQues.append(variablesDictionnary[index])
		finally:
			pass

	if elementsInQues.len() > 0 and variablesInQues.len() > 0 and valuesInQues.len() = 0:
		for variable in variablesInQues:
			for element in elementsInQues:
				return f"la {variable} de {getattr(element, name, 'elementNameError')} est {getattr(element, variable, 'variableValueError')}"
			for value in valuesInQues:
        return f"l'élément avec une {variable} de {value} est {getattr(element, variable, 'variableValueError')}"

next((x for x in test_list if x.value == value), None)
