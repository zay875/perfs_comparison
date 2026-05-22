
#include "fstream"
#include <cctype>
#include <iostream>
#include <map>
#include <string>
#include <vector>
// this code is used to merge the individual merged global stats from
// meged_global_stats_for_a_datset and gather all 8 in one csv
namespace {

std::string trim(const std::string &text) {
  const std::size_t start = text.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::string();
  }
  const std::size_t end = text.find_last_not_of(" \t\r\n");
  return text.substr(start, end - start + 1);
}

std::vector<std::string> parseStatsLine(const std::string &text) {
  std::vector<std::string> parts;
  const std::size_t commaPos = text.find(',');
  const std::size_t semicolonPos = text.find(';');
  const char delimiter =
      (semicolonPos != std::string::npos &&
       (commaPos == std::string::npos || semicolonPos < commaPos))
          ? ';'
          : ',';

  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t sep = text.find(delimiter, start);
    const std::string part = text.substr(
        start, sep == std::string::npos ? std::string::npos : sep - start);
    parts.push_back(trim(part));
    if (sep == std::string::npos) {
      break;
    }
    start = sep + 1;
  }

  while (parts.size() < 4) {
    parts.push_back("");
  }
  return parts;
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
  // Map: metric_code -> (metric_name, map of dataset_id -> (hexaly, mathopt))
  std::map<
      std::string,
      std::pair<std::string,
                std::map<std::string, std::pair<std::string, std::string>>>>
      metricsData;

  // Read all files and aggregate by metric

  for (const std::string &inputFile : inputFiles) {
    std::ifstream in(inputFile);
    if (!in.is_open()) {
      std::cerr << "[WARN] Could not open input CSV file, skipping: "
                << inputFile << "\n";
      continue;
    }

    const std::string datasetId = extractDatasetId(inputFile);
    std::string line;
    bool isHeader = true;

    while (std::getline(in, line)) {
      if (isHeader) {
        isHeader = false;
        continue;
      }
      if (trim(line).empty()) {
        continue;
      }

      const std::vector<std::string> parts = parseStatsLine(line);
      const std::string &metricCode = parts[0];
      const std::string &metricName = parts[1];
      const std::string &hexalyValue = parts[2];
      const std::string &mathoptValue = parts[3];

      if (metricsData.find(metricCode) == metricsData.end()) {
        metricsData[metricCode] = {metricName, {}};
      }
      metricsData[metricCode].second[datasetId] = {hexalyValue, mathoptValue};
    }

    in.close();
  }

  // Write output file
  std::string base = "perfs_csv/merged_stats/";
  std::ofstream out(base + outputFile);
  if (!out.is_open()) {
    std::cerr << "Error: Could not open output CSV file: " << outputFile
              << "\n";
    return;
  }

  // Write header
  out << "metric_code,metric_name";
  for (int i = 1; i <= 8; ++i) {
    out << ",hexaly_value_dataset_" << i << ",mathopt_value_dataset_" << i;
  }
  out << "\n";

  // Write data rows
  for (const auto &metricEntry : metricsData) {
    const std::string &metricCode = metricEntry.first;
    const std::string &metricName = metricEntry.second.first;
    const auto &datasetValues = metricEntry.second.second;

    out << metricCode << "," << metricName;

    for (int i = 1; i <= 8; ++i) {
      const std::string datasetId = std::to_string(i);
      auto it = datasetValues.find(datasetId);
      if (it != datasetValues.end()) {
        out << "," << it->second.first << "," << it->second.second;
      } else {
        out << ",,";
      }
    }
    out << "\n";
  }

  out.close();

  std::cout << "Merged CSV written to: " << base + outputFile << "\n";
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr
        << "Usage: " << argv[0]
        << " <output_csv> <global_stat_merg_csv1> [global_stat_merg_csv2 ...]"
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
