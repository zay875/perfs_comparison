set terminal png size 1200,600
set output 'plots/phase_solve_time_agg_ratio.png'
set title 'MathOpt/Hexaly Total Time Ratio by Dataset'
set xlabel 'Dataset'
set ylabel 'Ratio (MathOpt total / Hexaly total)'
set key autotitle columnhead
set datafile separator ','
set xtics rotate by -45
plot 'perfs_csv/hexaly_mathopt_perf.csv.phaseTime.agg_by_dataset.csv' using 0:11:xtic(1) with linespoints title 'Total Ratio'
