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
	if (
		len(elementsInQues) > 0
		and len(variablesInQues) > 0
		and len(valuesInQues) == 0
	):
		for variable in variablesInQues:
			for element in elementsInQues:
				element = elementsDictionary[element]
				print(
					f"la {variable} de "
					f"{getattr(element, 'name', 'elementNameError')} est "
					f"{getattr(
						element, variablesDictionary[variable], 'variableValueError'
					)}")
			for value in valuesInQues:
				print(
					f"l'élément avec une {variable} de {value} est"
					f"{''.join(
						x.name for x in list(elementsDictionary.values()) if x.variable == value
					)}")
