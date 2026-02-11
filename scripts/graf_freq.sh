#!/bin/bash

# 1. Validar que existe el archivo de entrada
if [ ! -f "$1" ]; then
    echo "Error: No encuentro el archivo $1"
    exit 1
fi

# 2. Definir nombres
INPUT_FILE="$1"
OUTPUT_FILE="${1%.*}.png"

# 3. Crear un archivo de comandos temporal para gnuplot
cat << EOF > cmd.gp
    set terminal pngcairo size 1000,600 font 'Arial,12'
    set output '$OUTPUT_FILE'
    
    unset key
    set logscale x
    set xrange [20:20000]
    set yrange [-15:15]
    set xlabel 'Frequency (Hz)'
    set ylabel 'Magnitude (dB)'

    # Tics exactos del artículo
    set xtics (30, 100, 300, 1000, 3000, 10000)
    set format x '' 
    set xtics add ('30' 30, '100' 100, '300' 300, '1k' 1000, '3k' 3000, '10k' 10000)

    # Rejilla
    set mxtics 10
    set grid xtics mxtics ytics lc rgb '#cccccc' lw 1, lc rgb '#eeeeee' lw 0.5

    # Dibujo
    plot '$INPUT_FILE' with lines lc rgb '#0060ad' lw 1.5
EOF

# 4. Ejecutar gnuplot usando el archivo de comandos
gnuplot cmd.gp

# 5. Limpiar
rm cmd.gp

echo "Gráfico generado: $OUTPUT_FILE"
