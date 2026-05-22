#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
// this code parses logs for hexaly and mathopt, creates a merge csv file of
// bothe solveur inofs, obj, bound, gap and solve phase time it plots phase
// solve time for mathopt and hexaly
// creates .data files for the gnuplots

struct TimeEntry {
  int datasetIndex = 0;
  std::string timeCount;
  std::vector<std::string> primal_bound; // 4 optimal values per dataset
  std::vector<std::string> dual_bound;   // gap for stored optimal values
  std::vector<std::string> obj;
  std::vector<std::string> bound;
  std::vector<std::string> solve_time;
  std::vector<std::string> iteration_time;
  // std::vector<std::string> feasibleObjectives; // all feasible values found
  // std::vector<std::string> feasibleGaps;       // gap for all feasible values
};

std::string extractValueAfterEquals(const std::string &line) {
  // Try finding '=' first (for "key = value" format)
  std::size_t eqPos = line.find('=');
  std::size_t colonPos =
      (eqPos == std::string::npos) ? line.rfind(':') : std::string::npos;

  std::size_t delim = (eqPos != std::string::npos) ? eqPos : colonPos;
  if (delim == std::string::npos) {
    return "";
  }
  std::size_t valStart = delim + 1;
  while (valStart < line.size() &&
         std::isspace(static_cast<unsigned char>(line[valStart]))) {
    ++valStart;
  }
  std::size_t valEnd = line.find_first_of(" \t\r\n,", valStart);
  return line.substr(valStart, valEnd == std::string::npos ? std::string::npos
                                                           : valEnd - valStart);
}

// use helper to extract token after marker
std::string extractAfterMarker(const std ::string &line,
                               const std::string &marker) {
  std::size_t pos = line.find(marker);
  if (pos == std::string::npos) {
    return "";
  }
  std::size_t start = pos + marker.size();
  while (start < line.size() &&
         std::isspace(static_cast<unsigned char>(line[start]))) {
    ++start;
  }
  std::size_t end = line.find_first_of(" \t\r\n", start);
  return line.substr(start, end == std::string::npos ? std::string::npos
                                                     : end - start);
}

// Extract value that appears after a specific key on the same log line,
// handling both "key: value" and "key = value" formats.
std::string extractValueAfterKey(const std::string &line,
                                 const std::string &key) {
  const std::size_t keyPos = line.find(key);
  if (keyPos == std::string::npos) {
    return "";
  }

  std::size_t pos = keyPos + key.size();
  while (pos < line.size() &&
         std::isspace(static_cast<unsigned char>(line[pos]))) {
    ++pos;
  }
  if (pos < line.size() && (line[pos] == ':' || line[pos] == '=')) {
    ++pos;
  }
  while (pos < line.size() &&
         std::isspace(static_cast<unsigned char>(line[pos]))) {
    ++pos;
  }

  const std::size_t end = line.find_first_of(" \t\r\n,}", pos);
  return line.substr(pos,
                     end == std::string::npos ? std::string::npos : end - pos);
}

std::string convertDurationToSeconds(const std::string &duration) {
  if (duration.empty()) {
    return "";
  }

  double totalSeconds = 0.0;
  std::size_t pos = 0;
  bool parsedAny = false;

  while (pos < duration.size()) {
    while (pos < duration.size() &&
           std::isspace(static_cast<unsigned char>(duration[pos]))) {
      ++pos;
    }
    if (pos >= duration.size()) {
      break;
    }

    std::size_t parsedLength = 0;
    double value = 0.0;
    try {
      value = std::stod(duration.substr(pos), &parsedLength);
    } catch (const std::exception &) {
      return duration;
    }

    if (parsedLength == 0) {
      return duration;
    }

    pos += parsedLength;
    std::size_t unitStart = pos;
    while (pos < duration.size() &&
           std::isalpha(static_cast<unsigned char>(duration[pos]))) {
      ++pos;
    }

    const std::string unit = duration.substr(unitStart, pos - unitStart);
    if (unit == "ms") {
      totalSeconds += value / 1000.0;
    } else if (unit == "s") {
      totalSeconds += value;
    } else if (unit == "m") {
      totalSeconds += value * 60.0;
    } else if (unit == "h") {
      totalSeconds += value * 3600.0;
    } else {
      return duration;
    }
    parsedAny = true;
  }

  if (!parsedAny) {
    return duration;
  }

  std::ostringstream out;
  out << std::fixed << std::setprecision(6) << totalSeconds;
  return out.str();
}
double durationToSeconds(const std::string &duration) {
  if (duration.find("ms") != std::string::npos) {
    return std::stod(duration) / 1000.0;
  }

  if (duration.find('m') != std::string::npos) {
    size_t mPos = duration.find('m');

    double minutes = std::stod(duration.substr(0, mPos));
    double seconds = std::stod(duration.substr(mPos + 1));

    return minutes * 60.0 + seconds;
  }

  return std::stod(duration);
}

std::vector<TimeEntry> parseLogsMathopt(const std::string &filename) {
  std::vector<TimeEntry> entries;
  std::ifstream file(filename);
  std::string line;

  const std::string marker1 = "TIME COUNT in ";
  const std::string marker2 = "Running dataset ";
  const std::string marker3 = "primal_bound";
  const std::string marker4 = "dual_bound";
  const std::string marker5 = "solve_time";
  TimeEntry current;
  bool haveDataset = false;
  bool justSawSummary = false;
  int CpsolverresponseCount = 0;

  while (std::getline(file, line)) {
    if (line.find(marker2) != std::string::npos) {
      if (haveDataset && !current.timeCount.empty()) {
        entries.push_back(current);
      }
      current = {};
      current.datasetIndex = std::stoi(extractAfterMarker(line, marker2));
      haveDataset = true;
      CpsolverresponseCount = 0;
      justSawSummary = false;
      continue;
    }
    if (!haveDataset) {
      continue;
    }

    if (line.find(marker3) != std::string::npos) {
      const std::string v = extractValueAfterKey(line, marker3);
      if (!v.empty()) {
        current.primal_bound.push_back(v);
      }
    }
    if (line.find(marker4) != std::string::npos) {
      const std::string v = extractValueAfterKey(line, marker4);
      if (!v.empty()) {
        current.dual_bound.push_back(v);
      }
    }
    if (line.find(marker5) != std::string::npos) {
      const std::string v = extractValueAfterKey(line, marker5);
      if (!v.empty()) {
        current.solve_time.push_back(v);
      }
    }

    if (line.find(marker1) != std::string::npos) {
      current.timeCount = extractAfterMarker(line, marker1);
      continue;
    }
  }
  if (haveDataset && !current.timeCount.empty()) {
    entries.push_back(current);
  }
  return entries;
}

std::vector<TimeEntry> parseLogsHexaly(const std::string &filename) {
  std::vector<TimeEntry> entries;
  std::ifstream file(filename);
  std::string line;

  const std::string marker1 = "TIME COUNT in ";
  const std::string marker2 = "Running dataset ";
  const std::string marker6 = "obj";
  const std::string marker7 = "bounds";
  const std::string marker8 = "iterations performed in";
  TimeEntry current;
  bool haveDataset = false;

  while (std::getline(file, line)) {
    if (line.find(marker2) != std::string::npos) {
      if (haveDataset && !current.timeCount.empty()) {
        entries.push_back(current);
      }
      current = {};
      current.datasetIndex = std::stoi(extractAfterMarker(line, marker2));
      haveDataset = true;
      continue;
    }
    if (!haveDataset) {
      continue;
    }
    // line immediately after "Optimal solution" contains "obj    = <value>"
    if (line.find(marker6) != std::string::npos &&
        line.find("obj") != std::string::npos &&
        line.find("objectives") == std::string::npos &&
        line.find('=') != std::string::npos) {
      current.obj.push_back(extractValueAfterEquals(line));
      continue;
    }

    if (line.find(marker7) != std::string::npos &&
        line.find('=') != std::string::npos) {
      current.bound.push_back(extractValueAfterEquals(line));
      continue;
    }
    if (line.find(marker8) != std::string::npos) {
      // Keep up to 5 values per dataset.
      if (current.iteration_time.size() < 5) {
        current.iteration_time.push_back(extractAfterMarker(line, marker8));
      }
      continue;
    }

    if (line.find(marker1) != std::string::npos) {
      current.timeCount = extractAfterMarker(line, marker1);
      continue;
    }
  }
  if (haveDataset && !current.timeCount.empty()) {
    entries.push_back(current);
  }
  return entries;
}

void writeComparisonCsv(const std::string &mathoptFile,
                        const std::vector<TimeEntry> &mathoptEntries,
                        const std::string &hexalyFile,
                        const std::vector<TimeEntry> &hexalyEntries,
                        const std::string &output) {
  const auto computeGap = [](const std::string &numeratorBase,
                             const std::string &numeratorSub,
                             const std::string &denominator) -> std::string {
    if (numeratorBase.empty() || numeratorSub.empty() || denominator.empty()) {
      return "";
    }

    try {
      const double base = std::stod(numeratorBase);
      const double sub = std::stod(numeratorSub);
      const double denom = std::stod(denominator);
      if (denom == 0.0) {
        return "";
      }

      std::ostringstream out;
      out << std::fixed << std::setprecision(6) << ((base - sub) / denom);
      return out.str();
    } catch (const std::exception &) {
      return "";
    }
  };

  // Build lookup maps by dataset index
  std::map<int, const TimeEntry *> mathoptMap, hexalyMap;
  for (const auto &e : mathoptEntries)
    mathoptMap[e.datasetIndex] = &e;
  for (const auto &e : hexalyEntries)
    hexalyMap[e.datasetIndex] = &e;

  // Collect all dataset indices in order
  std::vector<int> datasets;
  for (const auto &kv : mathoptMap)
    datasets.push_back(kv.first);
  for (const auto &kv : hexalyMap)
    if (!mathoptMap.count(kv.first))
      datasets.push_back(kv.first);
  std::sort(datasets.begin(), datasets.end());

  std::ofstream csv(output);
  // Two-row header: group row + column row
  csv << "dataset,hexaly solver,,,,mathopt solver,,,\n";
  csv << ",bound,obj,iteration_time,GAP_hexaky,primal_bound,dual_bound,solve_"
         "time,GAP-mathopt\n";

  for (int ds : datasets) {
    const TimeEntry *h = hexalyMap.count(ds) ? hexalyMap.at(ds) : nullptr;
    const TimeEntry *m = mathoptMap.count(ds) ? mathoptMap.at(ds) : nullptr;

    size_t rows = 0;
    if (h)
      rows = std::max(rows, h->bound.size());
    if (m)
      rows = std::max(rows, m->primal_bound.size());

    for (size_t i = 0; i < rows; ++i) {
      csv << ds << ",";
      // hexaly columns
      csv << (h && i < h->bound.size() ? h->bound[i] : "") << ",";
      csv << (h && i < h->obj.size() ? h->obj[i] : "") << ",";
      csv << (h && i < h->iteration_time.size() ? h->iteration_time[i] : "")
          << ",";
      csv << (h && i < h->obj.size() && i < h->bound.size()
                  ? computeGap(h->obj[i], h->bound[i], h->obj[i])
                  : "")
          << ",";
      // mathopt columns
      csv << (m && i < m->primal_bound.size() ? m->primal_bound[i] : "") << ",";
      csv << (m && i < m->dual_bound.size() ? m->dual_bound[i] : "") << ",";
      csv << (m && i < m->solve_time.size()
                  ? convertDurationToSeconds(m->solve_time[i])
                  : "")
          << ",";
      csv << (m && i < m->primal_bound.size() && i < m->dual_bound.size()
                  ? computeGap(m->primal_bound[i], m->dual_bound[i],
                               m->primal_bound[i])
                  : "")
          << "\n";
    }
  }
}
/*
void mergeGlobalStatsCsv(const std::string &hexalyFile,
                         const std::string &mathoptFile,
                         const std::string &outputFile) {
  std::ifstream hexaly_in(hexalyFile);
  std::ifstream mathopt_in(mathoptFile);
  std::ofstream out(outputFile);

  if (!hexaly_in.is_open() || !mathopt_in.is_open()) {
    std::cerr << "Error: Could not open input CSV files\n";
    return;
  }

  const auto trim = [](const std::string &text) {
    const std::size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
      return std::string();
    }
    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
  };

  const auto parseStatsLine = [&](const std::string &text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
      const std::size_t sep = text.find(';', start);
      const std::string part = text.substr(
          start, sep == std::string::npos ? std::string::npos : sep - start);
      parts.push_back(trim(part));
      if (sep == std::string::npos) {
        break;
      }
      start = sep + 1;
    }
    while (parts.size() < 3) {
      parts.push_back("");
    }
    return parts;
  };

  std::string line;

  std::getline(hexaly_in, line);
  std::getline(mathopt_in, line);

  out << "metric_code,metric_name,hexaly_value,mathopt_value\n";

  while (std::getline(hexaly_in, line)) {
    if (trim(line).empty()) {
      continue;
    }

    const std::vector<std::string> hexalyParts = parseStatsLine(line);
    std::string mathoptLine;
    std::vector<std::string> mathoptParts(3, "");
    if (std::getline(mathopt_in, mathoptLine) && !trim(mathoptLine).empty()) {
      mathoptParts = parseStatsLine(mathoptLine);
    }

    out << hexalyParts[0] << ',' << hexalyParts[1] << ',' << hexalyParts[2]
        << ',' << mathoptParts[2] << "\n";
  }

  hexaly_in.close();
  mathopt_in.close();
  out.close();

  std::cout << "Merged CSV written to: " << outputFile << "\n";
}
*/

void generatePlotHexaly(const std::string &filename,
                        const std::vector<TimeEntry> &entries) {
  const std::string outBase = "solve_phase_time_12_threads_hexaly";

  std::ofstream gnuplot("plots/" + outBase + ".gnuplot");
  gnuplot << "set terminal png size 800,600\n";
  gnuplot << "set output '" << outBase << ".png'\n";
  gnuplot << "set title 'Iteration Time" << filename << "'\n";
  gnuplot << "set xlabel 'Dataset'\n";
  gnuplot << "set ylabel 'Time'\n";
  gnuplot << "set xtics 1\n";
  gnuplot << "set key autotitle columnhead\n";
  gnuplot << "plot '../Data_logs/" << outBase
          << ".dat' using 1:12 with linespoints title columnhead(12), "
             "'' using 1:13 with linespoints title columnhead(13), "
             "'' using 1:14 with linespoints title columnhead(14), "
             "'' using 1:15 with linespoints title columnhead(15), "
             "'' using 1:16 with linespoints title columnhead(16)\n";
  gnuplot.close();

  std::ofstream datafile("Data_logs/" + outBase + ".dat");
  datafile << "dataset" << " bound_1 bound_2 bound_3 bound_4 bound_5"
           << " obj_1 obj_2 obj_3 obj_4" << " obj_5"
           << " iteration_time_1 iteration_time_2 iteration_time_3"
              " iteration_time_4 iteration_time_5\n";
  for (size_t i = 0; i < entries.size(); ++i) {
    datafile << entries[i].datasetIndex;

    for (size_t j = 0; j < entries[i].bound.size(); ++j) {
      datafile << " " << entries[i].bound[j];
    }

    for (size_t j = 0; j < entries[i].obj.size(); ++j) {
      datafile << " " << entries[i].obj[j];
    }

    for (size_t j = 0; j < entries[i].iteration_time.size(); ++j) {
      datafile << " " << entries[i].iteration_time[j];
    }
    for (size_t j = 0; j < entries[i].timeCount.size(); ++j) {
      datafile << " " << entries[i].timeCount[j];
    }

    datafile << "\n";
  }
  datafile.close();
  const std::string command_h = "cd plots && gnuplot " + outBase + ".gnuplot";
  int ret = system(command_h.c_str());
  if (ret != 0) {
    std::cerr << "Error executing gnuplot: " << ret << std::endl;
  } else {
    std::cout << "Generated plot: solve_time_hexaly.png" << std::endl;
  }
}
void generatePlotMathopt(const std::string &filename,
                         const std::vector<TimeEntry> &entries) {
  const std::string outBase = "solve_phase_time_12_threads_mathopt";

  std::ofstream gnuplot("plots/" + outBase + ".gnuplot");
  gnuplot << "set terminal png size 800,600\n";
  gnuplot << "set output '" << outBase << ".png'\n";
  gnuplot << "set title 'Solve Time" << filename << "'\n";
  gnuplot << "set xlabel 'Dataset'\n";
  gnuplot << "set ylabel 'Time'\n";
  gnuplot << "set xtics 1\n";
  gnuplot << "set key autotitle columnhead\n";
  gnuplot << "plot '../Data_logs/" << outBase
          << ".dat' using 1:12 with linespoints title columnhead(12), "
             "'' using 1:13 with linespoints title columnhead(13), "
             "'' using 1:14 with linespoints title columnhead(14), "
             "'' using 1:15 with linespoints title columnhead(15), "
             "'' using 1:16 with linespoints title columnhead(16)\n";
  gnuplot.close();

  std::ofstream datafile("Data_logs/" + outBase + ".dat");
  datafile
      << "dataset"
      << " primal_bound_1 primal_bound_2 primal_bound_3 primal_bound_4 "
         "primal_bound_5"
      << " dual_bound_1 dual_bound_2 dual_bound_3 dual_bound_4 dual_bound_5"
      << " solve_time_1 solve_time_2 solve_time_3 solve_time_4"
         " solve_time_5\n";
  for (size_t i = 0; i < entries.size(); ++i) {
    datafile << entries[i].datasetIndex;

    for (size_t j = 0; j < entries[i].primal_bound.size(); ++j) {
      datafile << " " << entries[i].primal_bound[j];
    }

    for (size_t j = 0; j < entries[i].dual_bound.size(); ++j) {
      datafile << " " << entries[i].dual_bound[j];
    }

    for (size_t j = 0; j < entries[i].solve_time.size(); ++j) {

      datafile << " " << durationToSeconds(entries[i].solve_time[j]);
    }
      for (size_t j = 0; j < entries[i].timeCount.size(); ++j) {
      datafile << " " << entries[i].timeCount[j];
    }

    datafile << "\n";
  }
  datafile.close();
  const std::string command_m = "cd plots && gnuplot " + outBase + ".gnuplot";
  int ret = system(command_m.c_str());
  if (ret != 0) {
    std::cerr << "Error executing gnuplot: " << ret << std::endl;
  } else {
    std::cout << "Generated plot: solve_time_mathopt.png" << std::endl;
  }
}

/*
void comparePlots(const std::string &file1, const std::string &file2) {
  std::map<int, std::string> file1ByDataset;
  std::map<int, std::string> file2ByDataset;

      file1ByDataset= read_csv(file1);


  for (const auto &entry : entries2) {
    if (!entry.feasibleObjective.empty()) {
      file2ByDataset[entry.datasetIndex] = entry.feasibleObjective;
    }
  }

  std::ofstream csv("comparison_last_values.csv");
  csv << "dataset," << file1 << "," << file2 << "\n";
  for (int dataset = 1; dataset <= 8; ++dataset) {
    csv << dataset << ",";
    if (file1ByDataset.count(dataset)) {
      csv << file1ByDataset[dataset];
    }
    csv << ",";
    if (file2ByDataset.count(dataset)) {
      csv << file2ByDataset[dataset];
    }
    csv << "\n";
  }
  csv.close();

  std::ofstream gnuplot("compare_last_values.gnuplot");
  gnuplot << "set terminal png size 1000,600\n";
  gnuplot << "set output 'comparison_last_values.png'\n";
  gnuplot << "set title 'Last Value Comparison'\n";
  gnuplot << "set xlabel 'Dataset'\n";
  gnuplot << "set ylabel 'Last Value'\n";
  gnuplot << "set xtics 1\n";
  gnuplot << "set datafile separator ','\n";
  gnuplot
      << "plot 'comparison_last_values.csv' using 1:2 with linespoints title '"
      << file1 << "', ";
  gnuplot << "'comparison_last_values.csv' using 1:3 with linespoints title '"
          << file2 << "'\n";
  gnuplot.close();
}
*/

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <mathopt_log_file> <hexaly_log_file> <hexaly_mathopt_perfs>"
              << std::endl;
    return 1;
  }

  std::string logfile1 = argv[1];
  std::string logfile2 = argv[2];
  std::string outputfile = argv[3];
  std::string outputfilecsv = "perfs_csv/" + outputfile;

  std::cout << "Parsing log files..." << std::endl;
  std::vector<TimeEntry> entries1 = parseLogsMathopt(logfile1);

  std::vector<TimeEntry> entries2 = parseLogsHexaly(logfile2);

  std::cout << "File 1 entries: " << entries1.size() << std::endl;
  std::cout << "File 2 entries: " << entries2.size() << std::endl;
  std::cout << "Generating comparison csv file into perfs_csv/" << std::endl;

  writeComparisonCsv(logfile1, entries1, logfile2, entries2, outputfilecsv);

  // Merge global stats CSVs
  /*
    mergeGlobalStatsCsv(
       "output_hexaly_files_logs/1/2025_04/stats/full_stats_global_stats.csv",
       "output_mathopt_files_logs/1/2025_04/stats/full_stats_global_stats.csv",
       "merged_global_stats.csv");
   */
  std::cout << "Generating plots of solve time for mathopt" << std::endl;
  std::cout << "Generating data file into Data_logs/ used to make gnuplots"
            << std::endl;
  generatePlotMathopt(logfile1, entries1);

  std::cout << "Generating plots of solve time for hexaly" << std::endl;
  std::cout << "Generating data file into Data_logs/ used to make gnuplots"
            << std::endl;

  generatePlotHexaly(logfile2, entries2);

  return 0;
}
