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
	for word in splitQues:
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
				nextWord =
				if word+1.index(variablesEnd)
				#if splitQues([word + 1]) in variablesEnd:
					variablesInQues.append(" ".join(splitQues[word], splitQues[word + 1]))
		finally:
			pass

	if elementsInQues.len() > 0 and variablesInQues.len() > 0 and valuesInQues.len() = 0:
		for element in elementsInQues:
			for variable in variablesInQues:
				return f"la {variable} de {getattr(element, name, )} est {getattr(element, variable, 'error')}"
