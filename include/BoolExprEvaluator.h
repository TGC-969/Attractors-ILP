#ifndef BOOLEXPREVALUATOR_H
#define BOOLEXPREVALUATOR_H

#include <string>
#include <unordered_map>

class BoolExprEvaluator {
public:
    // Evaluates the boolean expression using variable assignments (id -> 0/1)
    static bool evaluate(const std::string& expr, const std::vector<int>& inputs, const std::unordered_map<std::string, int>& nameToId);
    static bool evalExpr(const std::string& s, size_t& i, const std::vector<int>& inputs, const std::unordered_map<std::string, int>& nameToId);

private:
    static std::string parseToken(const std::string& s, size_t& i);
};

#endif
