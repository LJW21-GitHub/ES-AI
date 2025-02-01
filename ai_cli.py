from ai import treatment, analyze

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
	print(f"[BOT] : {treatment(question)}")
