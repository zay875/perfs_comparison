#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

vector<string> splitCSVLine(const string &line) {
  vector<string> result;
  string cell;
  bool inQuotes = false;

  for (char c : line) {
    if (c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ',' && !inQuotes) {
      result.push_back(cell);
      cell.clear();
    } else {
      cell += c;
    }
  }

  result.push_back(cell);
  return result;
}

string cleanNumber(string s) {

  // Remove quotes
  s.erase(remove(s.begin(), s.end(), '"'), s.end());

  // Replace comma by dot
  replace(s.begin(), s.end(), ',', '.');

  // Remove spaces
  s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());

  // Remove %
  s.erase(remove(s.begin(), s.end(), '%'), s.end());

  return s;
}

bool isNumber(string s) {

  s = cleanNumber(s);

  if (s.empty())
    return false;

  char *endptr = nullptr;

  strtod(s.c_str(), &endptr);

  return (*endptr == '\0');
}

double toDouble(string s) {

  s = cleanNumber(s);

  return stod(s);
}

int main() {

  string inputFile = "merged_mean_agg_res_stats_all_datasets.csv";
  string outputFile = "significant_differences.csv";

  ifstream fin(inputFile);
  ofstream fout(outputFile);

  if (!fin.is_open()) {
    cerr << "Cannot open input file\n";
    return 1;
  }

  string line;

  // Read header
  getline(fin, line);

  // Output header
  fout << "metric_code," << "metric_name," << "dataset," << "hexaly_value,"
       << "mathopt_value," << "percent_difference\n";

  const double THRESHOLD_PERCENT = 2.0;

  while (getline(fin, line)) {

    vector<string> cols = splitCSVLine(line);

    if (cols.size() < 18)
      continue;

    string metric_code = cols[0];
    string metric_name = cols[1];

    // Loop through datasets
    for (int dataset = 1; dataset <= 8; ++dataset) {

      int hexalyIndex = 2 + (dataset - 1) * 2;
      int mathoptIndex = hexalyIndex + 1;

      string hexalyStr = cols[hexalyIndex];
      string mathoptStr = cols[mathoptIndex];

      if (!isNumber(hexalyStr) || !isNumber(mathoptStr))
        continue;

      double hexaly = toDouble(hexalyStr);
      double mathopt = toDouble(mathoptStr);

      double diff = fabs(hexaly - mathopt);

      double reference = (fabs(mathopt) > 1e-9) ? fabs(mathopt) : 1.0;

      double percentDiff = (diff / reference) * 100.0;

      // Keep only significant differences
      if (percentDiff >= THRESHOLD_PERCENT) {

        fout << metric_code << "," << metric_name << "," << dataset << ","
             << hexaly << "," << mathopt << "," << percentDiff << "\n";
      }
    }
  }

  fin.close();
  fout.close();

  cout << "Filtered CSV written to: " << outputFile << endl;

  return 0;
}