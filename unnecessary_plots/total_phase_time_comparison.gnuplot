set terminal png size 1200,600
set output 'plots/phase_solve_time_agg_totals.png'
set title 'Aggregated Total Time by Dataset'
set xlabel 'Dataset'
set ylabel 'Total Time (seconds)'
set key autotitle columnhead
set datafile separator ','
set xtics rotate by -45
plot 'perfs_csv/hexaly_mathopt_perf_min_max_phases.csv' using 0:3:xtic(1) with linespoints title 'Hexaly Total', '' using 0:7:xtic(1) with linespoints title 'MathOpt Total'
