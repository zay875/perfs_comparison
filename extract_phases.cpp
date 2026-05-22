#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
//this code takes the hexaly_mathopt_perf csv and creates a csv aggregated by datsets with min and max solve phase
//it provides plots for plotting phase time for each solveur they are commented not necessary
std::string trim(const std::string &s) {
  const std::size_t first = s.find_first_not_of(" \t");
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = s.find_last_not_of(" \t");
  return s.substr(first, last - first + 1);
}

std::vector<std::string> splitCsvLine(const std::string &line) {
  std::vector<std::string> cols;
  std::stringstream ss(line);
  std::string token;
  while (std::getline(ss, token, ',')) {
    cols.push_back(trim(token));
  }
  return cols;
}

double parseMixedValue(const std::string &raw) {
  const std::string value = trim(raw);
  if (value.empty()) {
    return 0.0;
  }

  if (value.find(':') != std::string::npos) {
    int h = 0;
    int m = 0;
    int s = 0;
    char c1 = 0;
    char c2 = 0;
    std::stringstream ts(value);
    if (ts >> h >> c1 >> m >> c2 >> s && c1 == ':' && c2 == ':') {
      return static_cast<double>(h) + static_cast<double>(m) / 60.0 +
             static_cast<double>(s) / 3600.0;
    }
    return 0.0;
  }

  try {
    return std::stod(value);
  } catch (...) {
    return 0.0;
  }
}

// Extract basename from path
std::string basename(const std::string &path) {
  size_t pos = path.find_last_of("/\\");
  return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

// Normalize path - remove leading slash for relative paths
std::string normalizePath(const std::string &path) {
  if (!path.empty() && path[0] == '/') {
    // Check if it's a relative path with leading slash that should be relative
    return path.substr(1);
  }
  return path;
}

// Convert path to be relative from plots/ directory
std::string toPlotRelativePath(const std::string &path) {
  std::string normalized = normalizePath(path);
  return "../" + normalized;
}

void select_min_max_phase_and_plot(const std::string &selectedCsvFile) {
  std::filesystem::create_directories("../gnuplots");
  std::ifstream input(selectedCsvFile);
  if (!input) {
    std::cerr << "Cannot open selected CSV file: " << selectedCsvFile
              << std::endl;
    return;
  }

  struct AggStats {
    int count = 0;
    double hexalySum = 0.0;
    double mathoptSum = 0.0;
    double hexalyMin = 0.0;
    double hexalyMax = 0.0;
    double mathoptMin = 0.0;
    double mathoptMax = 0.0;
    int hexalyMinPhase = 1;
    int hexalyMaxPhase = 1;
    int mathoptMinPhase = 1;
    int mathoptMaxPhase = 1;
  };

  std::map<std::string, AggStats> aggByDataset;
  std::vector<std::string> datasetOrder;

  std::string line;
  std::getline(input, line); // skip header

  while (std::getline(input, line)) {
    const std::vector<std::string> cols = splitCsvLine(line);
    if (cols.size() < 3) {
      continue;
    }

    const std::string &dataset = cols[0];
    if (dataset.empty()) {
      continue;
    }
    const double hexalyTime = parseMixedValue(cols[1]);
    const double mathoptTime = parseMixedValue(cols[2]);

    if (!aggByDataset.count(dataset)) {
      datasetOrder.push_back(dataset);
      AggStats init;
      init.hexalyMin = hexalyTime;
      init.hexalyMax = hexalyTime;
      init.mathoptMin = mathoptTime;
      init.mathoptMax = mathoptTime;
      aggByDataset[dataset] = init;
    }

    AggStats &stats = aggByDataset[dataset];
    stats.count += 1;
    stats.hexalySum += hexalyTime;
    stats.mathoptSum += mathoptTime;
    if (hexalyTime < stats.hexalyMin) {
      stats.hexalyMin = hexalyTime;
      stats.hexalyMinPhase = stats.count;
    }
    if (hexalyTime > stats.hexalyMax) {
      stats.hexalyMax = hexalyTime;
      stats.hexalyMaxPhase = stats.count;
    }
    if (mathoptTime < stats.mathoptMin) {
      stats.mathoptMin = mathoptTime;
      stats.mathoptMinPhase = stats.count;
    }
    if (mathoptTime > stats.mathoptMax) {
      stats.mathoptMax = mathoptTime;
      stats.mathoptMaxPhase = stats.count;
    }
  }

  const std::string aggCsvFile =
      "perfs_csv/hexaly_mathopt_perf_min_max_phases.csv";
  std::ofstream aggOut(aggCsvFile);
  if (!aggOut) {
    std::cerr << "Cannot write aggregate CSV file: " << aggCsvFile << std::endl;
    return;
  }

  aggOut << "dataset,count,"
            "hexaly_sum,hexaly_mean,hexaly_min,hexaly_min_phase,hexaly_max,"
            "hexaly_max_phase,"
            "mathopt_sum,mathopt_mean,mathopt_min,mathopt_min_phase,mathopt_"
            "max,mathopt_max_phase,"
            "sum_ratio_mathopt_over_hexaly\n";

  aggOut << std::fixed << std::setprecision(6);
  for (const auto &dataset : datasetOrder) {
    const AggStats &stats = aggByDataset[dataset];
    const double hexalyMean =
        (stats.count > 0) ? (stats.hexalySum / static_cast<double>(stats.count))
                          : 0.0;
    const double mathoptMean =
        (stats.count > 0)
            ? (stats.mathoptSum / static_cast<double>(stats.count))
            : 0.0;
    const double ratio =
        (stats.hexalySum != 0.0) ? (stats.mathoptSum / stats.hexalySum) : 0.0;

    aggOut << dataset << ',' << stats.count << ',' << stats.hexalySum << ','
           << hexalyMean << ',' << stats.hexalyMin << ','
           << stats.hexalyMinPhase << ',' << stats.hexalyMax << ','
           << stats.hexalyMaxPhase << ',' << stats.mathoptSum << ','
           << mathoptMean << ',' << stats.mathoptMin << ','
           << stats.mathoptMinPhase << ',' << stats.mathoptMax << ','
           << stats.mathoptMaxPhase << ',' << ratio << "\n";
  }

  aggOut.close();
  /*
      {
    const std::string gnuplotFile =
        "gnuplots/total_phase_time_comparison.gnuplot";
    std::ofstream gnuplot(gnuplotFile);
    gnuplot << "set terminal png size 1200,600\n";
    gnuplot << "set output 'plots/phase_solve_time_agg_totals.png'\n";
    gnuplot << "set title 'Aggregated Total Time by Dataset'\n";
    gnuplot << "set xlabel 'Dataset'\n";
    gnuplot << "set ylabel 'Total Time (seconds)'\n";
    gnuplot << "set key autotitle columnhead\n";
    gnuplot << "set datafile separator ','\n";
    gnuplot << "set xtics rotate by -45\n";
    gnuplot << "plot '" << aggCsvFile
            << "' using 0:3:xtic(1) with linespoints title 'Hexaly Total', "
               "'' using 0:7:xtic(1) with linespoints title 'MathOpt Total'\n";
    gnuplot.close();

    const std::string command = "gnuplot " + gnuplotFile;
    int ret = system(command.c_str());
    if (ret != 0) {
      std::cerr << "Error executing gnuplot: " << ret << std::endl;
    } else {
      std::cout << "Generated plot: plots/phase_solve_time_agg_totals.png"
                << std::endl;
    }
  }
  */

  /*
    {
    const std::string gnuplotFile =
        "gnuplots/ratio_solve_time_comparison.gnuplot";
    std::ofstream gnuplot(gnuplotFile);
    gnuplot << "set terminal png size 1200,600\n";
    gnuplot << "set output 'plots/ratio_solve_time_comparison.png'\n";
    gnuplot << "set title 'MathOpt/Hexaly Total Time Ratio by Dataset'\n";
    gnuplot << "set xlabel 'Dataset'\n";
    gnuplot << "set ylabel 'Ratio (MathOpt total / Hexaly total)'\n";
    gnuplot << "set key autotitle columnhead\n";
    gnuplot << "set datafile separator ','\n";
    gnuplot << "set xtics rotate by -45\n";
    gnuplot << "plot '" << aggCsvFile
            << "' using 0:11:xtic(1) with linespoints title 'Total Ratio'\n";
    gnuplot.close();

    const std::string command = "gnuplot " + gnuplotFile;
    int ret = system(command.c_str());
    if (ret != 0) {
      std::cerr << "Error executing gnuplot: " << ret << std::endl;
    } else {
      std::cout << "Generated plot: plots/ratio_solve_time_comparison.png"
                << std::endl;
    }
  }
  */

}

void generateGapPlot(const std::string &filename) {
  std::string baseName = basename(filename);
  const std::string outBase = baseName + ".perfs";
  std::string normalized = normalizePath(filename);
  const std::string filteredCsv = filename + ".filtered.csv";

  {
    std::ifstream input(normalized);
    if (!input) {
      std::cerr << "Cannot open CSV file: " << normalized << std::endl;
      return;
    }

    std::ofstream output(filteredCsv);
    if (!output) {
      std::cerr << "Cannot write filtered CSV file: " << filteredCsv
                << std::endl;
      return;
    }

    // Read header lines
    std::string headerLine1, headerLine2;
    std::getline(input, headerLine1); // skip first header
    std::getline(input, headerLine2); // skip second header

    // Write simplified header with all columns
    output
        << "dataset,hexaly_bound,hexaly_obj,hexaly_iteration_time,hexaly_GAP,"
        << "mathopt_primal_bound,mathopt_dual_bound,mathopt_solve_time,"
        << "mathopt_GAP\n";

    // Accumulate last row seen for each dataset
    std::map<std::string, std::vector<std::string>> lastRowByDataset;
    std::vector<std::string> datasetOrder;

    std::string line;
    while (std::getline(input, line)) {
      const std::vector<std::string> cols = splitCsvLine(line);
      if (cols.size() < 9) {
        continue;
      }
      const std::string &ds = cols[0];
      if (!lastRowByDataset.count(ds)) {
        datasetOrder.push_back(ds);
      }
      lastRowByDataset[ds] = cols;
    }

    // Write one row per dataset (the last value seen)
    for (const auto &ds : datasetOrder) {
      const auto &cols = lastRowByDataset[ds];
      output << cols[0] << ",";
      for (size_t i = 1; i < cols.size() && i < 9; ++i) {
        output << cols[i];
        if (i < 8)
          output << ",";
      }
      output << "\n";
    }
  }

/*
std::string gnuplotFile = "gnuplots/GAP_comparison.gnuplot";
  std::ofstream gnuplot(gnuplotFile);
  gnuplot << "set terminal png size 1200,600\n";
  gnuplot << "set output 'plots/GAP_comparison.png'\n";
  gnuplot << "set title 'GAP Comparison'\n";
  gnuplot << "set xlabel 'Dataset'\n";
  gnuplot << "set ylabel 'GAP'\n";
  gnuplot << "set key autotitle columnhead\n";
  gnuplot << "set datafile separator ','\n";
  gnuplot << "plot '" << filteredCsv
          << "' using 1:5 with linespoints title 'Hexaly GAP', "
             "'' using 1:9 with linespoints title 'MathOpt GAP'\n";
  gnuplot.close();
  std::string command = "gnuplot " + gnuplotFile;
  int ret = system(command.c_str());
  if (ret != 0) {
    std::cerr << "Error executing gnuplot: " << ret << std::endl;
  } else {
    std::cout << "Generated plot: plots/GAP_comparison.png" << std::endl;
  }
*/
}
void generatePhaseSolveTimepPlot(const std::string &filename) {
  std::string baseName = basename(filename);
  const std::string outBase = baseName + ".phaseTime";
  std::string normalized = normalizePath(filename);
  const std::string filteredCsv = outBase + ".selected.csv";

  {
    std::ifstream input(normalized);
    if (!input) {
      std::cerr << "Cannot open CSV file: " << normalized << std::endl;
      return;
    }

    std::ofstream output(filteredCsv);
    if (!output) {
      std::cerr << "Cannot write filtered CSV file: " << filteredCsv
                << std::endl;
      return;
    }

    // Read header lines
    std::string headerLine1, headerLine2;
    std::getline(input, headerLine1); // skip first header
    std::getline(input, headerLine2); // skip second header

    // Write simplified header with all columns
    output << "dataset,hexaly_iteration_time," << "mathopt_solve_time\n";

    std::string line;
    while (std::getline(input, line)) {
      const std::vector<std::string> cols = splitCsvLine(line);
      if (cols.size() < 9) {
        continue;
      }
      // Keep all rows and only selected columns.
      output << cols[0] << "," << cols[3] << "," << cols[7] << "\n";
    }
  }

  std::string gnuplotFile = "gnuplots/phase_solve_time_comparison.gnuplot";

  std::string outputPng = "plots/phase_solve_time_comparison.png";
  std::ofstream gnuplot(gnuplotFile);
  std::cout << "writing file gnuplot to " << gnuplotFile;
  gnuplot << "set terminal png size 1200,600\n";
  gnuplot << "set output '" << outputPng << "'\n";
  gnuplot << "set title 'Hexaly Iteration Time vs MathOpt Solve Time'\n";
  gnuplot << "set xlabel 'Dataset'\n";
  gnuplot << "set ylabel 'Time (seconds)'\n";
  gnuplot << "set key autotitle columnhead\n";
  gnuplot << "set datafile separator ','\n";
  gnuplot << "plot '" << filteredCsv
          << "' using 1:2 with linespoints title 'Hexaly Iteration Time', "
             "'' using 1:3 with linespoints title 'MathOpt Solve Time'\n";
  gnuplot.close();
  std::string command = "gnuplot " + gnuplotFile;
  int ret = system(command.c_str());
  if (ret != 0) {
    std::cerr << "Error executing gnuplot: " << ret << std::endl;
  } else {
    std::cout << "Generated plot: " << outputPng << std::endl;
  }
}
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <csv_file1>" << std::endl;
    return 1;
  }

  std::string csvfile = argv[1];

  // Merge global stats CSVs
  /*
    mergeGlobalStatsCsv(
       "output_hexaly_files_logs/1/2025_04/stats/full_stats_global_stats.csv",
       "output_mathopt_files_logs/1/2025_04/stats/full_stats_global_stats.csv",
       "merged_global_stats.csv");
   */
  const std::string hexaly_mathopt_perf_csv = "perfs_csv/" + basename(csvfile);
  generateGapPlot(hexaly_mathopt_perf_csv);
  generatePhaseSolveTimepPlot(hexaly_mathopt_perf_csv);

  const std::string phaseSelectedCsv =
      "perfs_csv/" + basename(csvfile) + ".phaseTime.selected.csv";
  select_min_max_phase_and_plot(phaseSelectedCsv);

  return 0;
}
