class element:
	def __init__(self, name, masmol):
		self.name = name
		self.masmol = masmol

elementDictionnary = ["helium", "oxygene"]
variablesBeginning = ["masse"]
variableEnd = ["molaire"]

helium = element("Hélium", 23)
oxygene = element("Oxygène", 50)

def normalize(name):
	name = name.lower()
	name = name.replace("é", "e")
	name = name.replace("è", "e")

print("Nyaah~~ uwu")

while True:
	question = input("")
	questionalnum = list([val for val in question if val.isalnum()])
	splitQues = question.split(" ")
	elementsInQues = []
	valuesInQues = []
	variablesInQues =[]
	for word in splitQues:
		isElement = word.index(elementDictionnary)
		isVariable = word.index(elementBeginning)
		isValue = isValue.isnumeric()
		if isElement == True:
			elementsInQues.append(word) 
		if isValue == True:
			valuesInQues.append(word) 
		if isVariable == True:
			variablesInQues.append(word) 
	for 