from ai import treatment, isTraining, history, iterHistory, train
import sys

print("""----------
Bienvenue dans la version CLI de ce programme.
----------
Keywords :
train : passer en mode entraînement.
stop : arrêter l'entraînement, retourner en apprentissage.
exit : arrêter le programme
----------
[BOT] : Comment puis-je vous aider ?""")
while True:
	print("----------")
	question = input("[USER] : ")
	print("----------")
	if question == "train":
		train()
		print(f"[BOT] : {treatment("")}")
	elif question == "stop":
		isTraining = False
	elif question == "exit":
		sys.exit()
	else:
		print(f"[BOT] : {treatment(question)}")
