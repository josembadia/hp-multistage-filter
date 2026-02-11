import numpy as np
import matplotlib.pyplot as plt

import sys 

# ----------------------------------------------------
# 1. Obtener el nombre del archivo de la línea de comandos
# ----------------------------------------------------
if len(sys.argv) < 2:
    print("Uso: python tu_script.py <nombre_del_archivo.txt>")
    sys.exit(1)

file_input = sys.argv[1] # nombre del archivo con el resultado del filtrado

# 1. Cargar los datos
# Usamos skiprows=1 para ignorar la primera línea que contenga texto (si existe).
try:
    data = np.loadtxt(file_input, skiprows=1) 
except ValueError:
    print("Error de carga. Intentando sin saltar filas...")
    data = np.loadtxt(file_input)

N = len(data) # N será 9200

# 2. Definir el Eje X como el índice de las muestras
# La variable x_axis ahora es simplemente [0, 1, 2, ..., 9199]
x_axis = np.arange(N) 

# 3. Generar el gráfico de tallo (Stem Plot)
# Se mantiene el estilo de plot discreto. Se elimina el argumento obsoleto.
plt.figure(figsize=(12, 6)) 
plt.stem(x_axis, data, linefmt='k-', markerfmt='ko', basefmt='k:', label='Señal') 

# 4. Ajustar el estilo
# xlim abarca el rango completo de índices
plt.xlim(0, N) 
# ylim se ajusta automáticamente a los valores mínimo y máximo de tu vector
plt.ylim(np.min(data) * 1.1, np.max(data) * 1.1) 

plt.xlabel("Índice de la Muestra (n) [0 a 9199]")
plt.ylabel("Amplitud del Valor")
plt.title(f"Evolución de la Señal Completa (N={N} Muestras)")
plt.grid(True, axis='y', alpha=0.5) # Opcional: añadir rejilla para ver mejor los valores

# 5. Guardar el gráfico
# Generamos el nombre del PNG basado en el nombre del archivo de entrada
output_file = file_input.replace('.txt', '_plot.png')
plt.savefig(output_file, dpi=300, bbox_inches='tight')

plt.show()
