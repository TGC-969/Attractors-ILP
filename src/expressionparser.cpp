#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include "expressionparser.h"
#include <memory> // for std::unique_ptr

bool parseExpressions(
    const std::string& filename,
    std::unordered_map<std::string, std::shared_ptr<Node>>& nodes,
    std::unordered_map<std::string, int>& nameToId
) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    std::string line;
    int currentId = 0;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        size_t pos = line.find("*=");
        if (pos == std::string::npos) {
            std::cerr << "Skipping malformed line: " << line << std::endl;
            continue;
        }

        std::string name = line.substr(0, pos);
        std::string expr = line.substr(pos + 2);// after '*='


        nameToId[name] = currentId;
        nodes[name] = std::make_shared<Node>(currentId, name, expr);
        if(expr == name) nodes[name]->external = true;
        ++currentId;
    }

    infile.close();
    return true;
}














#include <fstream>
#include <sstream>
#include <iostream>

// bool parseExpressions(
//     const std::string& filename,
//     std::vector<Node>& nodes,
//     std::unordered_map<std::string, int>& nameToId
// ) {
//     std::ifstream infile(filename);
//     if (!infile.is_open()) {
//         std::cerr << "Error: Cannot open file " << filename << std::endl;
//         return false;
//     }
//
//     std::string line;
//     int currentId = 0;
//
//     while (std::getline(infile, line)) {
//         if (line.empty()) continue;
//
//         size_t pos = line.find("*=");
//         if (pos == std::string::npos) {
//             std::cerr << "Skipping malformed line: " << line << std::endl;
//             continue;
//         }
//
//         std::string name = line.substr(0, pos);
//         std::string expr = line.substr(pos + 2); // after '*='
//
//         nameToId[name] = currentId;
//         nodes.emplace_back(currentId, name, expr);
//         ++currentId;
//     }
//
//     infile.close();
//     return true;
// }
