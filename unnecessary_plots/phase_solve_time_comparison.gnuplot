set terminal png size 1200,600
set output 'plots/phase_solve_time_comparison.png'
set title 'Hexaly Iteration Time vs MathOpt Solve Time'
set xlabel 'Dataset'
set ylabel 'Time (seconds)'
set key autotitle columnhead
set datafile separator ','
plot 'hexaly_mathopt_perf.csv.phaseTime.selected.csv' using 1:2 with linespoints title 'Hexaly Iteration Time', '' using 1:3 with linespoints title 'MathOpt Solve Time'
