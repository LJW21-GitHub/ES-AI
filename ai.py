from assets.database import elemDict, varDict
import random
import matplotlib
import matplotlib.pyplot as plt
import os

"""
	NOTE: 
	Noms de fonctions : plus clair et plus long
	> analyze -> analyze_usr_string_chatbox

	Commentaires : 
	> Ajouter plus de commentaires sur quoi fait quoi.
	> Ajouter docstrings, type d'entrée et sortie.

	Noms de variables: plus clair et plus long 
	> Exemple : analyse : x, pas intuitif, plutôt indiquer :
	> x->usr_input_string

	try/except : 
	> Ne pas hésiter à utiliser le mot-clé raise.

	lisibilité : 
	> Espace ton code 

	(PS : Je parle mal, mais c'est quand même très très cool, les autres de ma promos
	ils utilisent tous ChatGPT comme des merdes.)

	Pistes d'amélioration : 
	- Éclaicit ton code, que ce soit côté commentaires ou docstrings.
	C'est, en soit peu important si tu gardes tout en local, mais si tu es sur un Repo Github, alors d'autres
	sont suceptibles de le voir.

	- Possibilité : utilise un fichier .bat pour faciliter le lancement de l'application par un autre utilisateur.
	ATTENTION : le chemin de tes fichiers reisque d'être cassé, utilise qqch du genre : 
	icon_path = os.path.join(os.path.dirname(__file__), '..',  'media', 'icons', 'app_icon.png')
	Car l'emplacement d'utilisation n'est plus le même.

	- Possibilité : Met à jour dynamiquement la taille de la fenêtre.
	Soit screen_height,screen_wide la hauteur et largeur d'écran.
	Utilise des multiplicateurs pour n%.

	- Possibilité : sur un côté esthétique plus simple, change le nom de la fenêtre et met une icône.
	Aussi, ajoute éventuellement un peu de son, c'est funny.
	Et, tu peux ajouter un temps de réponse avec un effet comme si l'autre paysan écrivait.

	- History : 
	Utilise une fonction write_to_history()
	qui écrit au propre ton message dans l'historique
	Éventuellement, utilise une structure pile/file; si utilisé dans le code.
	Voir  : https://www.ukonline.be/cours/python/apprendre-python/chapitre5-4
	Bref, l'utilisation d'une structure de stockage native (ou objet) serait bien plus propre.

	SI TU VEUX AVANCER :: https://www.france-ioi.org/algo/chapters.php
	
"""

isTraining = False
try:
	history = (open("assets/history").read()).split("#")
	while "" in history:
		history.remove("")
except FileNotFoundError:
	open("assets/history", "x")
	history = []
iterHistory = []


def analyze(x:str)->dict:
	"""
	Analyse une entrée utilisateur, et renvoie le dictionnaire approprié.
	"""
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


def treatment(entry):
	global history, iterHistory, output, answer, average, correct, wrong, isTraining
	if not isTraining:
		iterHistory = []
		entry = analyze(entry)
		for variable in entry["variables"]:
			for element in entry["elements"]:
				ans = elemDict[element][varDict[variable][2]]
				history.append(f"e/{variable}/{element}/{ans}")
				x = open("assets/history", "a")
				x.write(f"e/{variable}/{element}/{ans}#")
				x.close()
				# print(history)
				return (
					("la " if varDict[variable][3] == "f" else "le ")
					+ f"{varDict[variable][0]} "
					+ ("de l'" if element[0] in "aeiouyh" else "du ")
					+ f"{elemDict[element][0]} est "
					f"{ans} "
					+ varDict[variable][1]
				)
			for value in entry["values"]:
				diff = {}
				for i, x in elemDict.items():
					diff[str(i)] = abs(
						x[varDict[variable][2]]
						- value
					)
				# print(diff)
				ans = "".join(list(
					i for i, x in diff.items() if min(diff.values()) == x))
				history.append(f"v/{variable}/{value}/{ans}")
				x = open("assets/history", "a")
				x.write(f"v/{variable}/{value}/{ans}#")
				x.close()
				elemDict[ans][0]
				return (
					"l'élément avec une "
					+ varDict[variable][0]
					+ " de "
					+ f"{str(elemDict[ans][varDict[variable][2]])} "
					+ varDict[variable][1]
					+ (" est l'" if ans[0] in "aeiouyh" else " est le ")
					+ elemDict[ans][0]
				)
	elif isTraining:
		output = ""
		if iterHistory == []:
			history = list(set((history)))
			random.shuffle(history)
			iterHistory = iter(history)
			correct = 0
			wrong = 0
		else:
			entry = analyze(entry)
			for i in entry.values():
				for x in i:
					try:
						if answer in x:
							output = "correct ! "
							correct += 1
						else:
							output = "faux ! "
							wrong += 1
					except TypeError:
						if answer in i:
							output = "correct ! "
							correct += 1
						else:
							output = "faux ! "
							wrong += 1
		x = next(iterHistory, "END")
		if not x == "END":
			x = x.split("/")
			if x[0] == "e":
				output = (
					output
					+ ("Quelle " if varDict[x[1]][3] == "f" else "Quel ")
					+ ("est l'" if x[1][0] in "aeiouyh" else (
						"est la " if varDict[x[1]][3] == "f" else "est le "))
					+ varDict[x[1]][0]
					+ (" de l'" if x[2][0] in "aeiouyh" else " du ")
					+ elemDict[x[2]][0]
					+ " ?"
				)
				answer = float(x[3])
				# try:
				#	answer = int(answer)
				# except ValueError:
				#	pass
			elif x[0] == "v":
				output = (
					output
					+ (
						"Quel élément a une " if varDict[x[1]][3] == "f"
						else "Quel élément a un ")
					+ varDict[x[1]][0]
					+ " de "
					+ x[2]
					+ " ?"
				)
				answer = x[3]
			else:
				os.remove("assets/history")
				return "An error has occurred while trying to fetch history data. \
					deleting history file."
		elif x == "END":
			history = []
			x = open("assets/history", "w")
			x = x.write("")
			# x.close()
			correct100 = round(correct * 100 / (wrong + correct))
			fig, ax = plt.subplots()
			ax.pie([correct, wrong], labels=["correct", "faux"])
			plt.show()
			isTraining = False
			output = (output + f"""

bonnes réponses : {correct}
mauvaises réponses : {wrong}
taux de bonne réponses : {correct100}%""")
		else:
			return "something wrong happened. not supposed to be possible tho." #utilise le "Except as e", et print e en fstring : 'err_msg {e}'
		return output


def train(): #fix de flemmard, peut-être faire un fichier
	# qui contient tes globales, chargé avant ? 
	# A toi de voir.
	# + nom de fonction, plutôt change_training_state()->None:
	global isTraining
	isTraining = True


def pltUseQt():
	matplotlib.use('Qt5Agg')


def pltUseTk():
	matplotlib.use('TkAgg')
