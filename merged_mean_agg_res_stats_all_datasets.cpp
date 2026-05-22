
#include "fstream"
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
// this code is used to merge the mean_agg_res_stats from 1 to 8 and output them
// in one csv file
namespace {

std::string trim(const std::string &text) {
  const std::size_t start = text.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::string();
  }
  const std::size_t end = text.find_last_not_of(" \t\r\n");
  return text.substr(start, end - start + 1);
}

std::vector<std::string> parseStatsLine(const std::string &line) {

  std::vector<std::string> cols;

  std::stringstream ss(line);

  std::string cell;

  while (std::getline(ss, cell, ',')) {

    cols.push_back(trim(cell));
  }

  return cols;
}

std::string baseName(const std::string &path) {
  const std::size_t slashPos = path.find_last_of("/");
  if (slashPos == std::string::npos) {
    return path;
  }
  return path.substr(slashPos + 1);
}

bool isDigits(const std::string &text) {
  if (text.empty()) {
    return false;
  }
  for (char ch : text) {
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
  }
  return true;
}

std::string extractDatasetId(const std::string &filePath) {
  const std::string name = baseName(filePath);
  const std::size_t dotPos = name.rfind(".csv");
  const std::size_t underscorePos = name.rfind('_');

  if (dotPos != std::string::npos && underscorePos != std::string::npos &&
      underscorePos + 1 < dotPos) {
    const std::string suffix =
        name.substr(underscorePos + 1, dotPos - underscorePos - 1);
    if (isDigits(suffix)) {
      return suffix;
    }
  }

  if (dotPos != std::string::npos) {
    return name.substr(0, dotPos);
  }
  return name;
}

} // namespace
void mergeGlobalStatsCsv(const std::vector<std::string> &inputFiles,
                         const std::string &outputFile) {

  struct MetricRow {

    std::string metricCode;
    std::string metricName;

    std::map<int, std::pair<std::string, std::string>> datasetValues;
  };

  // preserve order
  std::vector<std::string> rowOrder;

  // unique row key -> row
  std::map<std::string, MetricRow> rows;

  // count occurrences
  std::map<std::string, int> occurrenceCounter;

  for (const std::string &inputFile : inputFiles) {

    std::ifstream in(inputFile);

    if (!in.is_open()) {
      std::cerr << "Cannot open file: " << inputFile << "\n";
      continue;
    }

    const int datasetId = std::stoi(extractDatasetId(inputFile));

    std::string line;

    bool firstLine = true;

    // reset occurrence count per dataset
    occurrenceCounter.clear();

    while (std::getline(in, line)) {

      if (firstLine) {
        firstLine = false;
        continue;
      }

      if (trim(line).empty()) {
        continue;
      }

      std::vector<std::string> parts = parseStatsLine(line);

      if (parts.size() < 4) {
        continue;
      }

      const std::string metricCode = trim(parts[0]);

      const std::string metricName = trim(parts[1]);

      const std::string hexalyValue = trim(parts[2]);

      const std::string mathoptValue = trim(parts[3]);

      // base metric identifier
      const std::string baseKey = metricCode + "||" + metricName;

      // occurrence index
      const int occurrence = occurrenceCounter[baseKey]++;

      // full unique row key
      const std::string fullKey = baseKey + "||" + std::to_string(occurrence);

      // first appearance
      if (rows.find(fullKey) == rows.end()) {

        MetricRow row;

        row.metricCode = metricCode;
        row.metricName = metricName;

        rows[fullKey] = row;

        rowOrder.push_back(fullKey);
      }

      // assign dataset values
      rows[fullKey].datasetValues[datasetId] = {hexalyValue, mathoptValue};
    }

    in.close();
  }

  // WRITE OUTPUT
  std::string base = "perfs_csv/merged_stats/";
  std::ofstream out(base + outputFile);

  if (!out.is_open()) {
    std::cerr << "Cannot open output file\n";
    return;
  }

  out << "metric_code,metric_name";

  for (int i = 1; i <= 8; ++i) {

    out << ",hexaly_value_dataset_" << i << ",mathopt_value_dataset_" << i;
  }

  out << "\n";

  for (const std::string &key : rowOrder) {

    const MetricRow &row = rows[key];

    out << row.metricCode << "," << row.metricName;

    for (int i = 1; i <= 8; ++i) {

      auto it = row.datasetValues.find(i);

      if (it != row.datasetValues.end()) {

        out << "," << it->second.first << "," << it->second.second;

      } else {

        out << ",###,###";
      }
    }

    out << "\n";
  }

  out.close();

  std::cout << "Merged CSV written to: " << base + outputFile << "\n";
}
int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <output_csv> <merged_stats_csv1> [merged_stats_csv2 ...]"
              << std::endl;
    return 1;
  }

  const std::string outputFile = argv[1];
  std::vector<std::string> inputFiles;
  inputFiles.reserve(static_cast<std::size_t>(argc - 2));
  for (int i = 2; i < argc; ++i) {
    inputFiles.emplace_back(argv[i]);
  }

  mergeGlobalStatsCsv(inputFiles, outputFile);
  return 0;
}
