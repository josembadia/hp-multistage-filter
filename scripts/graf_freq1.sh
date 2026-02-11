gnuplot -e "set terminal pngcairo size 1000,600; set output 'figura11a_limpia.png'; \
            set logscale x; set grid xtics mxtics ytics; \
            set xlabel 'Frecuencia (Hz)'; set ylabel 'Ganancia (dB)'; \
            set xrange [20:20000]; set yrange [-15:15]; \
            plot 'frecuencia.txt' with lines lc rgb '#0060ad' lw 1.5 title 'Respuesta en Frecuencia (Zigzag)'"
