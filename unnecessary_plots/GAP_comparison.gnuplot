set terminal png size 1200,600
set output 'plots/GAP_comparison.png'
set title 'GAP Comparison'
set xlabel 'Dataset'
set ylabel 'GAP'
set key autotitle columnhead
set datafile separator ','
plot 'perfs_csv/hexaly_mathopt_perf.csv.filtered.csv' using 1:5 with linespoints title 'Hexaly GAP', '' using 1:9 with linespoints title 'MathOpt GAP'
