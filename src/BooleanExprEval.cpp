#include "BoolExprEvaluator.h"
#include <stack>
#include <cctype>
#include <string>
#include <stdexcept>

int getPrecedence(char op) {
    if (op == '~') return 3;
    if (op == '&') return 2;
    if (op == '|') return 1;
    return 0;
}

bool applyOp(char op, bool a, bool b = false) {
    if (op == '&') return a & b;
    if (op == '|') return a | b;
    throw std::runtime_error("Invalid binary operator");
}

bool applyNot(bool val) {
    return !val;
}

// Read variable like Tbx5_2 or A
std::string parseVariable(const std::string& s, size_t& i) {
    std::string var;
    while (i < s.size() && (std::isalnum(s[i]) || s[i] == '_')) {
        var += s[i++];
    }
    if (var.empty()) throw std::runtime_error("Expected variable at position " + std::to_string(i));
    return var;
}

bool BoolExprEvaluator::evalExpr(const std::string& s, size_t& i, const std::vector<int>& inputs, const std::unordered_map<std::string, int>& nameToId) {
    std::stack<bool> values;
    std::stack<char> ops;

    while (i < s.length()) {
        if (s[i] == ' ') {
            ++i;
            continue;
        }

        if (s[i] == '(') {
            ops.push(s[i]);
            ++i;
        }
        else if (s[i] == ')') {
            while (!ops.empty() && ops.top() != '(') {
                char op = ops.top(); ops.pop();

                if (op == '~') {
                    bool v = values.top(); values.pop();
                    values.push(applyNot(v));
                } else {
                    bool b = values.top(); values.pop();
                    bool a = values.top(); values.pop();
                    values.push(applyOp(op, a, b));
                }
            }
            if (!ops.empty() && ops.top() == '(') ops.pop();
            ++i;
        }
        else if (s[i] == '~') {
            // Right-associative NOT
            ops.push(s[i]);
            ++i;
        }
        else if (s[i] == '&' || s[i] == '|') {
            char currentOp = s[i++];
            while (!ops.empty() && getPrecedence(ops.top()) >= getPrecedence(currentOp)) {
                char op = ops.top(); ops.pop();
                if (op == '~') {
                    bool v = values.top(); values.pop();
                    values.push(applyNot(v));
                } else {
                    bool b = values.top(); values.pop();
                    bool a = values.top(); values.pop();
                    values.push(applyOp(op, a, b));
                }
            }
            ops.push(currentOp);
        }
        else {
            std::string var = parseVariable(s, i);
            if (nameToId.find(var) == nameToId.end()) throw std::runtime_error("Unknown variable: " + var);
            int idx = nameToId.at(var);
            values.push(inputs[idx]);
        }
    }

    // Final evaluation
    while (!ops.empty()) {
        char op = ops.top(); ops.pop();
        if (op == '~') {
            bool v = values.top(); values.pop();
            values.push(applyNot(v));
        } else {
            bool b = values.top(); values.pop();
            bool a = values.top(); values.pop();
            values.push(applyOp(op, a, b));
        }
    }

    if (values.size() != 1) throw std::runtime_error("Malformed Boolean expression");
    return values.top();
}