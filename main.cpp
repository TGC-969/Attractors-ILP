#include "Node.h"
#include <iostream>
#include <unordered_map>
#include <string>

/*int main() {
    // Define the Boolean expression and node info
    std::string expr = "(A|C)";
    std::string nodeName = "Output";
    int nodeId = 2; // Assume A=0, B=1, C=2, so Output=3

    // Map of names to IDs
    std::unordered_map<std::string, int> nameToId = {
        {"A", 0},
        {"C", 1},
    };

    // Create node and solve threshold
    Node outputNode(nodeId, nodeName, expr);
    outputNode.solveThresholdFunction(3, nameToId); // total network size = 4

    // Extract and print result
    auto thresholdFn = outputNode.getThresholdFunction();
    const auto& weights = thresholdFn.first;
    int threshold = thresholdFn.second;

    std::cout << "Threshold function for node \"" << nodeName << "\":" << std::endl;
    std::cout << "Weights: ";
    for (size_t i = 0; i < weights.size(); ++i)
        std::cout << "w[" << i << "] = " << weights[i] << "  ";
    std::cout << "\nThreshold: T = " << threshold << std::endl;

    return 0;
}*/
#include "expressionParser.h"

int main() {
    // std::vector<Node> nodes;
    // std::unordered_map<std::string, int> nameToId;
    //
    // if (parseExpressions("../src/expressions_clean.txt", nodes, nameToId)) {
    //     std::cout << "Parsed " << nodes.size() << " nodes:\n";
    //     for (const auto& node : nodes) {
    //         std::cout << "ID: " << node.id << ", Name: " << node.name
    //                   << ", Expr: " << node.expr << "\n";
    //     }
    // }

    return 0;
}