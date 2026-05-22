set terminal png size 800,600
set output 'solve_phase_time_4_threads_mathopt.png'
set title 'Solve Timeoutput_mathopt_4_threads/run_all_datasets_mathopt_threads_4.log'
set xlabel 'Dataset'
set ylabel 'Time'
set xtics 1
set key autotitle columnhead
plot '../Data_logs/solve_phase_time_4_threads_mathopt.dat' using 1:12 with linespoints title columnhead(12), '' using 1:13 with linespoints title columnhead(13), '' using 1:14 with linespoints title columnhead(14), '' using 1:15 with linespoints title columnhead(15), '' using 1:16 with linespoints title columnhead(16)
