
#include "fstream"
#include <iostream>
#include <string>
#include <vector>
// this code is used to merge global stats for each dataset in the output files,
// wich means, merge (hexaly, mathopt) stat for dataset1 = merged stat for
// dataste 1

void mergeGlobalStatsCsv(const std::string &hexalyFile,
                         const std::string &mathoptFile,
                         const std::string &outputFile) {
  std::ifstream hexaly_in(hexalyFile);
  std::ifstream mathopt_in(mathoptFile);
  std::string base = "perfs_csv/merged_stats/global_stats/";
  std::ofstream out(base + outputFile);

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

  std::string hexalyLine, mathoptLine;

  while (true) {
    // Read next valid metric line from hexaly (skip empty lines and headers)
    while (std::getline(hexaly_in, hexalyLine)) {
      const std::string trimmed = trim(hexalyLine);
      if (trimmed.empty() || trimmed.find(';') == std::string::npos) {
        continue; // Skip empty lines and non-metric lines
      }
      const std::vector<std::string> parts = parseStatsLine(hexalyLine);
      if (!parts[0].empty() && !parts[2].empty()) {
        break; // Found a valid metric line
      }
    }
    if (hexaly_in.eof()) {
      break;
    }

    // Read next valid metric line from mathopt (skip empty lines and headers)
    while (std::getline(mathopt_in, mathoptLine)) {
      const std::string trimmed = trim(mathoptLine);
      if (trimmed.empty() || trimmed.find(';') == std::string::npos) {
        continue; // Skip empty lines and non-metric lines
      }
      const std::vector<std::string> parts = parseStatsLine(mathoptLine);
      if (!parts[0].empty() && !parts[2].empty()) {
        break; // Found a valid metric line
      }
    }
    if (mathopt_in.eof()) {
      break;
    }

    const std::vector<std::string> hexalyParts = parseStatsLine(hexalyLine);
    const std::vector<std::string> mathoptParts = parseStatsLine(mathoptLine);

    out << hexalyParts[0] << ',' << hexalyParts[1] << ',' << hexalyParts[2]
        << ',' << mathoptParts[2] << "\n";
  }

  hexaly_in.close();
  mathopt_in.close();
  out.close();

  std::cout << "Merged CSV written to: "
            << "perfs_csv/merged_stats/global_stats/" + outputFile << "\n";
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <stat_full_global_hexaly_csv1> <stat_full_global_mathopt_csv2> [output_csv]"
              << std::endl;
    return 1;
  }

  std::string stat_csv_hexaly = argv[1];
  std::string stat_csv_mathopt = argv[2];
  std::string outputFile = argc >= 4 ? argv[3] : "merged_global_stats1.csv";

  mergeGlobalStatsCsv(stat_csv_hexaly, stat_csv_mathopt, outputFile);
  return 0;
}
