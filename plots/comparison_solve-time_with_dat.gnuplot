# plot_logs.gnuplot
# Usage:
#   gnuplot plot_logs.gnuplot

set datafile separator whitespace

set terminal pngcairo size 1000,600 enhanced font "Arial,12"
set output "comparison_plot_solve_time.png"

set title "MathOpt vs Hexaly solve time Comparison"
set xlabel "Dataset"
set ylabel "Solve time"
set ydata time
set timefmt "%H:%M:%S"
set format y "%H:%M:%S"
set grid
set key outside

# Optional: use logarithmic scale if values vary a lot
# uncomment the next line if needed
# set logscale y

plot \
    "../Data_logs/run_all_datasets_mathopt.log.dat" using 0:1 with linespoints linewidth 1 pointtype 7 lc rgb "#00AA00"title "MathOpt", \
    "../Data_logs/run_all_datasets_hexaly.log.dat" using 0:1 with linespoints linewidth 1 pointtype 5 lc rgb "#8A2BE2"title "Hexaly"