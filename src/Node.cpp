#include "Node.h"
#include "BoolExprEvaluator.h"
#include "gurobi_c++.h"
#include <iostream>
#include <cmath>
#include <regex>
#include <unordered_set>
#include <symengine/basic.h>
#include <symengine/symbol.h>
#include <symengine/parser.h>
#include <symengine/simplify.h>
#include <symengine/integer.h>
#include <symengine/visitor.h>


class SymbolCollector : public SymEngine::BaseVisitor<SymbolCollector> {
public:
    std::set<std::string> symbols;

    // Correct visitor method implementation
    void bvisit(const SymEngine::Symbol &x) {
        symbols.insert(x.get_name());
    }

    // Base case for other types
    void bvisit(const SymEngine::Basic &x) {
        // Iterate through components for non-symbol expressions
        for (const auto &arg : x.get_args()) {
            arg->accept(*this);
        }
    }
};


Node::Node(int id, const std::string& name, const std::string& boolean_function_str)
        : id(id), name(remove_chars(name, " ")),
          expr(boolean_function_str), static_flag(false){

    // Parse and process boolean function
    boolean_function = SymEngine::parse(boolean_function_str);
    original_boolean_function = simplify(boolean_function);
    boolean_function = original_boolean_function;

    // Check if external (function equals name after removing certain chars)
    std::string processed_func = remove_chars(boolean_function_str, " ()");
    external = (processed_func == this->name);
}

std::vector<std::string> Node::get_boolean_functions_free_symbols(bool with_external = false) const {
        if (!with_external && external) return {};

        SymbolCollector collector;
        boolean_function->accept(collector);

        return {
            collector.symbols.begin(),
            collector.symbols.end()
        };
}

std::vector<std::string> Node::get_parents() const {
    auto symbols = get_boolean_functions_free_symbols();
    std::sort(symbols.begin(), symbols.end());
    return symbols;
}

void Node::update_boolean_function(const std::map<std::string, SymEngine::RCP<const SymEngine::Basic>>& assignment) {
    SymEngine::map_basic_basic sub_map;

    // Build substitution map of Symbols to Boolean expressions
    for (const auto& [node_name, boolean_expr] : assignment) {
        sub_map[SymEngine::symbol(node_name)] = boolean_expr;
    }

    // Perform substitution and simplification
    boolean_function = simplify(boolean_function->subs(sub_map));

    // Additional simplification to resolve nested substitutions
    boolean_function = SymEngine::simplify(expand(boolean_function));
}

SymEngine::RCP<const SymEngine::Basic> Node::assign_values_to_boolean_function(const std::map<std::string, int>& assignment) const {

        SymEngine::map_basic_basic sub_map;
        for (const auto& [var, val] : assignment) {
            sub_map[SymEngine::symbol(var)] = SymEngine::integer(val);
        }

        auto substituted = boolean_function->subs(sub_map);
        auto simplified = simplify(substituted);

        // First check if we have a direct integer result
        if (is_a<SymEngine::Integer>(*simplified)) {
            int int_value = rcp_static_cast<const SymEngine::Integer>(simplified)->as_int();
            return SymEngine::integer(int_value != 0 ? 1 : 0);
        }

        // Check for remaining symbols using our collector
        SymbolCollector collector;
        simplified->accept(collector);

        if (collector.symbols.empty()) {
            // No symbols remaining - try to resolve to boolean value
            try {
                // Attempt to evaluate through expansion
                auto expanded = expand(simplified);
                if (is_a<SymEngine::Integer>(*expanded)) {
                    int int_value = rcp_static_cast<const SymEngine::Integer>(expanded)->as_int();
                    return SymEngine::integer(int_value != 0 ? 1 : 0);
                }

                // Final check for zero equivalence
                if (expanded->__eq__(*SymEngine::integer(0))) {
                    return SymEngine::integer(0);
                }
                return SymEngine::integer(1);
            }
            catch (...) {
                // Fallback if expansion fails
                return simplified->__eq__(*SymEngine::integer(0)) ? SymEngine::integer(0) : SymEngine::integer(1);
            }
        }

        // If symbols remain, return the simplified expression
        return simplified;

}



void Node::solveThresholdFunction(int networkSize, const std::unordered_map<std::string, int>& nameToId) {
    std::vector<std::string> involvedVars;
    for (const auto& [name, id] : nameToId) {
        if (expr.find(name) != std::string::npos)
            involvedVars.push_back(name);
    }

    int numInputs = involvedVars.size();
    int numCombinations = 1 << numInputs;

    try {
        GRBEnv env = GRBEnv(true);
        env.set(GRB_IntParam_OutputFlag, 0); // silent mode
        env.start();
        GRBModel model = GRBModel(env);

        // Create weight variables (bounded to ±infinity)
        std::vector<GRBVar> weights, absWeights;
        for (int i = 0; i < networkSize; ++i) {
            GRBVar w = model.addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_INTEGER, "w_" + std::to_string(i));
            GRBVar abs_w = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "abs_w_" + std::to_string(i));
            weights.push_back(w);
            absWeights.push_back(abs_w);

            model.addConstr(abs_w >= w);
            model.addConstr(abs_w >= -w);
        }

        // Create threshold variable
        GRBVar thresholdVar = model.addVar(-GRB_INFINITY, GRB_INFINITY, 0, GRB_INTEGER, "T");

        // Add output constraints for all input combinations
        for (int mask = 0; mask < numCombinations; ++mask) {
            std::unordered_map<std::string, bool> inputValues;
            std::vector<int> inputBinary(networkSize, 0);

            for (int j = 0; j < numInputs; ++j) {
                bool bit = (mask >> j) & 1;
                inputValues[involvedVars[j]] = bit;
                inputBinary[nameToId.at(involvedVars[j])] = bit;
            }
            size_t i = 0;
            bool output = BoolExprEvaluator::evalExpr(expr, i, inputBinary, nameToId);

            GRBLinExpr lhs = 0;
            for (int k = 0; k < networkSize; ++k)
                lhs += inputBinary[k] * weights[k];

            if (output == 1)
                model.addConstr(lhs >= thresholdVar);
            else
                model.addConstr(lhs <= thresholdVar - 1);
        }

        // Objective: minimize sum of absolute weights
        GRBLinExpr obj = 0;
        for (const auto& abs_w : absWeights)
            obj += abs_w;

        model.setObjective(obj, GRB_MINIMIZE);
        model.optimize();

        // Store solved weights and threshold
        std::vector<int> solvedWeights(networkSize);
        for (int i = 0; i < networkSize; ++i)
            solvedWeights[i] = static_cast<int>(round(weights[i].get(GRB_DoubleAttr_X)));

        int solvedThreshold = static_cast<int>(round(thresholdVar.get(GRB_DoubleAttr_X)));

        threshold = std::make_pair(solvedWeights, solvedThreshold);
    } catch (GRBException& e) {
        std::cerr << "Gurobi Error: " << e.getMessage() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred while solving threshold function.\n";
    }
}

std::vector<std::string> Node::getParents() const
{
        std::unordered_set<std::string> uniqueNames;
        std::regex varRegex(R"([a-zA-Z_][a-zA-Z0-9_]*)");
        auto words_begin = std::sregex_iterator(expr.begin(), expr.end(), varRegex);
        auto words_end = std::sregex_iterator();

        for (auto it = words_begin; it != words_end; ++it) {
            std::string token = it->str();
            if (token != "and" && token != "or" && token != "not") {
                uniqueNames.insert(token);
            }
        }
        return {uniqueNames.begin(), uniqueNames.end()};
}
