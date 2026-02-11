#!/usr/bin/env python3

import pandas as pd
import argparse

def calcular_media_columna(file_path, column_number):
    # Leer el archivo de texto en un DataFrame de pandas
    df = pd.read_csv(file_path, delim_whitespace=True)
    
    # Verificar que el número de columna sea válido
    if column_number < 1 or column_number > len(df.columns):
        raise ValueError("El número de columna debe estar entre 1 y 6.")
    
    # Seleccionar la columna correcta (ajustar por índice de 0 en pandas)
    columna = df.iloc[:, column_number - 1]
    
    # Calcular la media de la columna seleccionada
    media = columna.mean()
    
    return media

def main():
    # Crear el analizador de argumentos
    parser = argparse.ArgumentParser(description='Calcular la media de una columna específica en un archivo de texto.')
    parser.add_argument('file_path', type=str, help='Ruta al archivo de texto.')
    parser.add_argument('column_number', type=int, help='Número de la columna para calcular la media (1-6).')
    
    # Parsear los argumentos
    args = parser.parse_args()
    
    # Calcular la media de la columna especificada
    try:
        media = calcular_media_columna(args.file_path, args.column_number)
        print(f"La media de la columna {args.column_number} es: {media}")
    except ValueError as e:
        print(e)

if __name__ == '__main__':
    main()
