from assets.database import elemDict, varDict, keyWords


def analyze(x):
	analysis = {
		"elements": [],
		"variables": [],
		"values": [],
		"type": "",
	}
	x = x.lower()
	x = x.replace("é", "e")
	x = x.replace("è", "e")
	x = x.replace("'", " ")
	for i, c in enumerate(x):
		if (
			c == ","
			and ((x[i - 1]) in "0123456789" if i > 0 else False)
			and ((x[i + 1]) in "0123456789" if i < len(x) - 1 else False)
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
		analysis["type"] = "question"
	return analysis


def treatment(question):
	# question = input("")
	# print(question)
	question = analyze(question)
	# print(question)
	if question["type"] == "question":
		for variable in question["variables"]:
			for element in question["elements"]:
				return (
					("la " if varDict[variable][3] == "f" else "le ")
					+ f"{varDict[variable][0]} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elemDict[element][0]} est "
					f"{elemDict[element][varDict[variable][2]]} "
					+ (varDict[variable][1])
				)
			for value in question["values"]:
				diff = {}
				for i, x in elemDict.items():
					diff[str(i)] = abs(
						x[varDict[variable][2]]
						- value
					)
				# print(diff)
				ans = "".join(list(
					i for i, x in diff.items() if min(diff.values()) == x))
				return (
					"l'élément avec une "
					+ varDict[variable][0]
					+ " de "
					+ f"{str(elemDict[ans][varDict[variable][2]])} "
					+ varDict[variable][1]
					+ (" est l'" if ans[0] in "aeiouyh" else " est le ")
					+ elemDict[ans][0]
				)
