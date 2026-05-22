# plot_logs.gnuplot
# Usage:
#   gnuplot plot_logs.gnuplot

set datafile separator whitespace

set terminal pngcairo size 1000,600 enhanced font "Arial,12"
set output "comparison_plot_objective_12_vs_4_vs_8_threads.png"

set title "MathOpt vs Hexaly objective Comparison"
set xlabel "Dataset"
set ylabel "Objective value"
set grid
set key outside

# Optional: use logarithmic scale if values vary a lot
# uncomment the next line if needed
# set logscale y

plot \
    "/home/ztaieb/Bureau/perfs_comparison/Data_logs/solve_time_mathopt.dat" using 1:5 with linespoints linewidth 1 pointtype 7 lc rgb "#00AA00"title "MathOpt_8_threads", \
    "/home/ztaieb/Bureau/perfs_comparison/Data_logs/solve_time_hexaly.dat" using 1:10 with linespoints linewidth 1 pointtype 5 lc rgb "#8A2BE2"title "Hexaly_8_threads", \
    "../Data_logs/solve_phase_time_4_threads_mathopt.dat" using 1:5 with linespoints linewidth 1 pointtype 7 lc rgb "#058194"title "MathOpt_4_thread", \
    "../Data_logs/solve_phase_time_4_threads_hexaly.dat" using 1:10 with linespoints linewidth 1 pointtype 5 lc rgb "#e43710"title "Hexaly_4_thread", \
        "../Data_logs/solve_phase_time_12_threads_mathopt.dat" using 1:5 with linespoints linewidth 1 pointtype 7 lc rgb "#180bcb"title "MathOpt_12_thread", \
    "../Data_logs/solve_phase_time_12_threads_hexaly.dat" using 1:10 with linespoints linewidth 1 pointtype 5 lc rgb "#f893d3"title "Hexaly_12_thread"
