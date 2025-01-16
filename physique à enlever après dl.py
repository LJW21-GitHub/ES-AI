#!C:\Python36\python.exe
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import linregress

list_M = [300, 400, 550, 600, 700] # en g
list_v_200ms = [0.40, 0.34, 0.32, 0.31, 0.28] # en m/s
list_v_500ms = [0.81, 0.68, 0.59, 0.56, 0.51] # en m/s

list_Metm = [] # en kg
list_dv = [] # en m/s
list_dv_sur_dt = [] # en m/s²


#Boucle for à compléter :
for i, x in enumerate(list_M):
    list_Metm.append((x+251)/1000)
    list_dv.append(list_v_500ms[i]-list_v_200ms[i])
    list_dv_sur_dt.append(list_dv[-1]/0.3)


plt.scatter(list_Metm, list_dv_sur_dt, marker='+')
plt.grid(True)
plt.title("Evolution de dv/dt en fonction de 1/m")
plt.ylabel("dv/dt (m/s²)")
plt.xlabel("1/m (kg-1)")
# Tracé de la courbe de modélisation :
regression = linregress(list_Metm, list_dv_sur_dt)
print(f"Pente = {regression[0]}")
print(f"Ordonnée à l'origine = {regression[1]}")
x=np.linspace(0, 1, 1000)
y=regression[0]*x+regression[1]
plt.plot(x,y,'-')


plt.show()

