set terminal png size 800,600
set output 'solve_time_hexaly.png'
set title 'Iteration Timerun_all_datasets_hexaly.log'
set xlabel 'Dataset'
set ylabel 'Time'
set xtics 1
set key autotitle columnhead
plot '../Data_logs/solve_time_hexaly.dat' using 1:12 with linespoints title columnhead(12), '' using 1:13 with linespoints title columnhead(13), '' using 1:14 with linespoints title columnhead(14), '' using 1:15 with linespoints title columnhead(15), '' using 1:16 with linespoints title columnhead(16)
