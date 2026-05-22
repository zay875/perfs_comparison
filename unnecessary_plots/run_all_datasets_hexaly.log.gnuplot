set terminal png size 800,600
set output 'run_all_datasets_hexaly.log.png'
set title 'Time Count Distribution - run_all_datasets_hexaly.log'
set xlabel 'Datasets'
set ylabel 'Time count'
set xtics rotate by 45
set ydata time
set timefmt "%H:%M:%S"
set format y "%H:%M:%S"
plot 'run_all_datasets_hexaly.log.dat' using 2:1 with linespoints title 'run_all_datasets_hexaly.log'
