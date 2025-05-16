#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <symengine/expression.h>

class Node {

public:
    int id;                                      // Unique node ID
    std::string name;                            // Node name
    std::string expr;                            // Boolean expression
    std::pair<std::vector<int>, int> threshold; // (weights, threshold)
    bool external;
    SymEngine::RCP<const SymEngine::Basic> boolean_function;
    SymEngine::RCP<const SymEngine::Basic> original_boolean_function;
    bool static_flag;
    std::vector<std::string> parents;


    Node(int _id, const std::string& _name, const std::string& _expr);

    std::pair<std::vector<int>, int> getThresholdFunction() const {
        return threshold;
    }

    void solveThresholdFunction(
        int networkSize,
        const std::unordered_map<std::string, int>& nameToId
    );

    std::vector<std::string> getParents() const;

    std::vector<std::string> get_boolean_functions_free_symbols(bool with_external) const;
    std::vector<std::string> get_parents() const;
    void update_boolean_function(const std::map<std::string, SymEngine::RCP<const SymEngine::Basic>>& assignment);
    SymEngine::RCP<const SymEngine::Basic> assign_values_to_boolean_function(const std::map<std::string, int>& assignment) const;
    bool is_self_assignment(const std::string& function_body) const;

    static std::string remove_chars(std::string str, const std::string& chars) {
        str.erase(std::remove_if(str.begin(), str.end(),
            [&chars](char c) { return chars.find(c) != std::string::npos; }),
            str.end());
        return str;
    }

};

#endif // NODE_H
