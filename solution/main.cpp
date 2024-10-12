#include <iostream>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <set>

using namespace std;
using json = nlohmann::json;

string readJsonFile(const string& jsonFileLocation) {
    ifstream file(jsonFileLocation);
    if (!file) {
        cerr << "Error opening file: " << jsonFileLocation << endl;
        return "";
    }

    string readJson;
    string line;
    while (getline(file, line)) {
        readJson += line;
    }
    file.close();

    return readJson;
}

void flattenJSON(const json& j, vector<map<string, string>>& flatData, map<string, string> currentData = {}, const string& parentKey = "") {
    for (auto it = j.begin(); it != j.end(); ++it) {
        string key = parentKey.empty() ? it.key() : parentKey + "." + it.key();
        
        if (it->is_structured()) {
            if (it->is_array()) {
                for (size_t i = 0; i < it->size(); ++i) {
                    flattenJSON((*it)[i], flatData, currentData, key + "[" + to_string(i) + "]");
                }
            } else {
                flattenJSON(*it, flatData, currentData, key);
            }
        } else {
            currentData[key] = it->is_null() ? "null" : it->dump();
        }
    }

    if (!currentData.empty()) {
        flatData.push_back(currentData);
        cout << "Added to flatData. Current flatData size: " << flatData.size() << endl;
    }
}

void writeToCSV(const vector<map<string, string>>& flatData, const string& csvFileLocation) {

    if (flatData.empty()) {
        cerr << "No data to write to CSV." << endl;
        return;
    }

    ofstream file(csvFileLocation);
    if (!file) {
        cerr << "Error opening file: " << csvFileLocation << endl;
        return;
    }

    /* The csv structure should be as follows:
     * region,region_warehouse_coord,region_warehouse_name,amount,date,vaccine_type,doses_required,manufacturer,storage_conditions,
     * Currently the flatData output looks as follows:
     *  regional_vaccine_supply[x].region: "North Verdania"
     *  regional_vaccine_supply[x].region_warehouse_coord: "(51.5074, -0.1278)"
     *  regional_vaccine_supply[x].region_warehouse_name: "Arctic Depot"
     *  regional_vaccine_supply[x].supply_details[x].supply_dates[x].amount: 3000
     *  regional_vaccine_supply[x].supply_details[x].supply_dates[x].date: "2024-01-01"
     *  Which means we the following keys:
        *  - region
        *  - region_warehouse_coord
        *  - region_warehouse_name
        *  - supply_details[x].supply_dates[x].amount
        *  - supply_details[x].supply_dates[x].date
        *  We need to map these keys to the csv structure.
     * This should all ideally be done withouth hardcoding the keys.
     */

    set<string> headerKeys;
    for (const auto& row : flatData) {
        for (const auto& [key, value] : row) {
            headerKeys.insert(key);
        }
    }

    vector<string> headers;
    for (const auto& key : headerKeys) {
        size_t lastDot = key.find_last_of(".");
        string lastKey = key.substr(lastDot + 1);
        if (find(headers.begin(), headers.end(), lastKey) == headers.end()) {
            headers.push_back(lastKey);
            file << lastKey << ",";
        }
    }
    file << endl;

    for (const auto& row : flatData) {
        for (const auto& header : headers) {
            bool found = false;
            for (const auto& [key, value] : row) {
                size_t lastDot = key.find_last_of(".");
                string lastKey = key.substr(lastDot + 1);
                if (lastKey == header) {
                    file << value << ",";
                    found = true;
                    break;
                }
            }
            if (!found) {
                file << ",";
            }
        }
        file << endl;
    }

    file.close();
}

int main() {
    string jsonFileLocation = "../../data/health-data.json";
    string readJson = readJsonFile(jsonFileLocation);

    if (readJson.empty()) {
        cerr << "Failed to read JSON from file." << endl;
        return 1;
    }

    try {
        json data = json::parse(readJson);
        vector<map<string, string>> flatData;
        flattenJSON(data, flatData);

        // cout << "Size of flatData: " << flatData.size() << endl;

        // for (const auto& row : flatData) {
        //     for (const auto& [key, value] : row) {
        //         cout << key << ": " << value << endl;
        //     }
        //     cout << endl;
        // }
        writeToCSV(flatData, "../../data/health-data.csv");
    } catch (const json::exception& e) {
        cerr << "JSON parsing error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
