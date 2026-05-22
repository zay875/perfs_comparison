# plot_logs.gnuplot
# Usage:
#   gnuplot plot_logs.gnuplot

set datafile separator whitespace

set terminal pngcairo size 1000,600 enhanced font "Arial,12"
set output "comparison_plot_solve_time_12_threads_vs_8_threads.png"

set title "MathOpt vs Hexaly solve time Comparison for 12 threads"
set xlabel "Dataset"
set ylabel "Solve time_mathopt_12"
set ylabel "Solve time_hexaly_12"
set ylabel "Solve time_mathopt_8"
set ylabel "Solve time_hexaly_8"
set ylabel "Solve time_mathopt_4"
set ylabel "Solve time_hexaly_4"
set ydata time
set timefmt "%H:%M:%S"
set format y "%H:%M:%S"
set grid
set key outside

# Optional: use logarithmic scale if values vary a lot
# uncomment the next line if needed
# set logscale y

plot \
    "../Data_logs/run_all_datasets_mathopt.log_12_threads.dat" using 0:1 with linespoints linewidth 1 pointtype 7 lc rgb "#00AA00"title "Solve time_mathopt_12", \
    "../Data_logs/run_all_datasets_hexaly.log_12_threads.dat" using 0:1 with linespoints linewidth 1 pointtype 5 lc rgb "#8A2BE2"title "Solve time_hexaly_12", \
    "../Data_logs/run_all_datasets_mathopt.log.dat" using 0:1 with linespoints linewidth 1 pointtype 7 lc rgb "#2958e8"title "Solve time_mathopt_8", \
    "../Data_logs/run_all_datasets_hexaly.log.dat" using 0:1 with linespoints linewidth 1 pointtype 5 lc rgb "#db21ce"title "Solve time_hexaly_8", \
    "../Data_logs/run_all_datasets_mathopt.log_4_threads.dat" using 0:1 with linespoints linewidth 1 pointtype 7 lc rgb "#0de6e6"title "Solve time_mathopt_4", \
    "../Data_logs/run_all_datasets_hexaly.log_4_threads.dat" using 0:1 with linespoints linewidth 1 pointtype 5 lc rgb "#f4ad21"title "Solve time_hexaly_4"