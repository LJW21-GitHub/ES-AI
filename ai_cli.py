from ai import treatment, isTraining, history, iterHistory, train, pltUseTk
import os

print("""---------------
   Bienvenue dans la version CLI de ce programme.
---------------
   Keywords :
   train   passer en mode entraînement.
   stop    arrêter l'entraînement, retourner en apprentissage.
   exit    arrêter le programme
---------------
[BOT]    Comment puis-je vous aider sur le tableau periodique ?""")
pltUseTk()
while not False: #while True, c'est pour les faibles.
	print("---------------")
	question = input("[USER]   ")
	print("---------------")
	if question == "train":
		train()
		os.system('cls' if os.name == 'nt' else 'clear')
		print(f"[BOT]    {treatment("")}")
	elif question == "stop":
		isTraining = False
	elif question == "exit":
		os.sys.exit()
	else:
		print(f"[BOT]    {treatment(question)}")
