#include <vector>
#include <string>
#include <gurobi_c++.h>

#include "BooleanNetwork.h"
#include "SolutionObjects.h"
#include "IncludingSolutions.cpp"
const int M = 50000;

struct ILPModel {
    GRBModel model;
    std::vector<GRBVar> states_vars;
    std::vector<GRBVar> externals_vars;
    std::vector<GRBVar> fixed_vars;
};

ILPModel build_ilp_model(BooleanNetwork& network, int size) {
    GRBEnv env;
    GRBModel model(env);
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);

    int state_size = network.get_state_size();
    int external_size = network.external_size;

    // Create variables
    std::vector<GRBVar> states_vars(state_size);
    std::vector<GRBVar> externals_vars(external_size);
    std::vector<GRBVar> fixed_vars(state_size);

    for (int i = 0; i < state_size; ++i) {
        states_vars[i] = model.addVar(0, 1, 0, GRB_BINARY, "states_" + std::to_string(i));
        fixed_vars[i] = model.addVar(0, 1, 0, GRB_BINARY, "fixed_" + std::to_string(i));
    }

    for (int i = 0; i < external_size; ++i) {
        externals_vars[i] = model.addVar(0, 1, 0, GRB_BINARY, "external_" + std::to_string(i));
    }

    // Main constraints
    for (int s_idx = 0; s_idx < state_size; ++s_idx) {
        auto node_thresholds_funcs = network.threshold_functions[s_idx];
        int threshold_order = 1;

        std::vector<GRBVar> always_over(threshold_order);
        std::vector<GRBVar> always_under(threshold_order);

        for (int order = 0; order < threshold_order; ++order) {
            always_over[order] = model.addVar(0, 1, 0, GRB_BINARY,
                "s" + std::to_string(s_idx) + "_always_over_" + std::to_string(order));
            always_under[order] = model.addVar(0, 1, 0, GRB_BINARY,
                "s" + std::to_string(s_idx) + "_always_under_" + std::to_string(order));

            auto [weights_dict, func_threshold] = node_thresholds_funcs;
            std::vector<int> p_indices;
            for(int i = 0; i < weights_dict.size(); ++i)
            {
                if(weights_dict[i] != 0) p_indices.push_back(i);
            }
            // x_min variables and constraints
            std::vector<GRBVar> x_min(p_indices.size());
            for (size_t i = 0; i < p_indices.size(); ++i) {
                x_min[i] = model.addVar(0, 1, 0, GRB_BINARY,
                    "s" + std::to_string(s_idx) + "_" + std::to_string(order) + "_min_" + std::to_string(i));
                int p_idx = p_indices[i];

                if (p_idx < state_size) {
                    // (fixed[p_idx] == 1) => x_min[i] == states[p_idx]
                    model.addGenConstrIndicator(fixed_vars[p_idx], 1, x_min[i] - states_vars[p_idx], GRB_EQUAL, 0.0);

                    // (fixed[p_idx] == 0) => x_min[i] == (weights < 0 || p_idx == s_idx)
                    bool target = (weights_dict.at(p_idx) < 0) || (p_idx == s_idx);
                    model.addGenConstrIndicator(fixed_vars[p_idx], 0, x_min[i], GRB_EQUAL, target ? 1.0 : 0.0);
                }
            }

            // y_min constraint
            GRBLinExpr y_min;
            for (size_t i = 0; i < p_indices.size(); ++i) {
                int p_idx = p_indices[i];
                double weight = weights_dict.at(p_idx);
                if (p_idx < state_size) {
                    y_min += x_min[i] * weight;
                } else {
                    int ext_idx = p_idx - state_size;
                    y_min += externals_vars[ext_idx] * weight;
                }
            }
            model.addConstr(y_min <= func_threshold - 1 + M * always_over[order],
                "s" + std::to_string(s_idx) + "_o" + std::to_string(order) + "_always_over_1");
            model.addConstr(y_min >= func_threshold - M * (1 - always_over[order]),
                "s" + std::to_string(s_idx) + "_o" + std::to_string(order) + "_always_over_2");

            // Similar x_max and y_max constraints would be implemented here
        }
            GRBVar or_var = model.addVar(0, 1, 0, GRB_BINARY, "or_" + std::to_string(s_idx));
            GRBVar or_terms[] = {always_over[0], always_under[0]};
            model.addGenConstrOr(or_var, or_terms, 2);
            model.addConstr(fixed_vars[s_idx] == or_var);
            model.addConstr(states_vars[s_idx] == always_over[0]);
    }

    // Static nodes constraints
    for (const auto& node_name : network.state_nodes_names) {
        const Node& node = *network.nodes[node_name];
        if (node.static_flag) {
            model.addConstr(fixed_vars[node.id] >= 1);
        }
    }

    // Fixed variables sum constraint
    GRBLinExpr sum_fixed;
    for (const auto& var : fixed_vars) sum_fixed += var;
    model.addConstr(sum_fixed == size);

    model.update();
    return ILPModel{std::move(model), states_vars, externals_vars, fixed_vars};
}

void add_stable_state_constraint(
    GRBModel& model,
    const std::vector<GRBVar>& states_vars,
    const std::vector<GRBVar>& fixed_vars,
    const std::map<int, int>& stable_states,
    bool stable_state
) {
    GRBLinExpr expr_1;

    // Build expr_1: sum over stable_states entries
    for (const auto& [i, val] : stable_states) {
        if (val == 1) {
            expr_1 += 1 - states_vars[i];
        } else {
            expr_1 += states_vars[i];
        }
    }

    if (stable_state) {
        model.addConstr(expr_1 >= 1, "compair");
        return;
    }

    // Build expr_2: sum over all states
    GRBLinExpr expr_2;
    for (int i = 0; i < states_vars.size(); ++i) {
        if (stable_states.count(i)) {
            expr_2 += 1 - fixed_vars[i];
        } else {
            expr_2 += fixed_vars[i];
        }
    }

    model.addConstr(expr_1 + expr_2 >= 1, "compair");
}
std::map<int, int> get_stable_states(
    GRBModel& model,
    int state_size,
    int external_size
) {
    std::map<int, int> stable_states;
    GRBVar* all_vars = model.getVars();

    // Extract state values (first 'state_size' variables)
    std::vector<int> states_val(state_size);
    for (int i = 0; i < state_size; ++i) {
        states_val[i] = static_cast<int>(std::round(all_vars[i].get(GRB_DoubleAttr_X)));
    }

    // Extract fixed values (starts after state_size + external_size)
    std::vector<int> fixed_val(state_size);
    const int fixed_start = state_size + external_size;
    for (int i = 0; i < state_size; ++i) {
        fixed_val[i] = static_cast<int>(std::round(all_vars[fixed_start + i].get(GRB_DoubleAttr_X)));
    }

    // Build stable states map
    for (int idx = 0; idx < state_size; ++idx) {
        if (fixed_val[idx] == 1) {
            stable_states[idx] = states_val[idx];
        }
    }

    delete[] all_vars;
    return stable_states;
}

SolutionObjects find_stable_states(BooleanNetwork& network) {
    SolutionObjects solutions;
    int state_size = network.get_state_size();

    // Iterate from state_size down to 1
    for (int i = state_size; i >= 1; --i) {
        // Build the ILP model for current size
        ILPModel ilp_model = build_ilp_model(network, i);
        bool fix_attractor = (i == state_size);

        // Find all solutions for current model
        while (true) {
            ilp_model.model.optimize();

            // Check optimization status
            int status = ilp_model.model.get(GRB_IntAttr_Status);
            if (status != GRB_OPTIMAL) {
                break;
            }

            // Get and store solution
            auto stable_states = get_stable_states(ilp_model.model, state_size, network.external_size);
            solutions.add_solution(stable_states);

            // Add exclusion constraint for next iteration
            add_stable_state_constraint(
                ilp_model.model,
                ilp_model.states_vars,
                ilp_model.fixed_vars,
                stable_states,
                fix_attractor
            );
        }
    }

    // Handle empty case
    if (solutions.solutions.empty()) {
        solutions.add_solution({});
    }

    return solutions;
}

SolutionObjects find_stable_states_and_external(
    BooleanNetwork& network,
    bool is_verify_sub_solutions = false,
    bool is_save = false
)
{
    // Print network info
    std::cout << "Network: " << network.name
              << ", state size: " << network.state_size
              << ", external: " << network.external_size
              << ", hole: " << network.hole_nodes_names.size()
              << ", external dependent: " << network.external_only_depended_nodes_names.size()
              << std::endl;

    SolutionObjects solutions = find_stable_states(network);
    solutions = SpecialNodes::find_external_assignments(network, solutions);

    // Count solutions by size
    int size_one_count = 0;
    int bigger_than_one_count = 0;
    for (const auto& solution : solutions.solutions) {
        if (solution.second.stable_nodes.size() == network.get_state_size()) {
            size_one_count++;
        } else if (solution.second.stable_nodes.size() < network.get_state_size()) {
            bigger_than_one_count++;
        }
    }
    std::cout << "size one solutions: " << size_one_count << std::endl;
    std::cout << "bigger than one solutions: " << bigger_than_one_count << std::endl;

    remove_included_solutions(network, solutions, is_verify_sub_solutions);

    if (!network.external_only_depended_nodes_names.empty()) {
        solutions = SpecialNodes::add_external_only_dependent_to_solutions(network, solutions);
    }

    if (!network.hole_nodes_names.empty()) {
        solutions = SpecialNodes::add_hole_to_solutions(network, solutions);
        std::cout << "size: " << solutions.solutions.size() << std::endl;
    }

    // Counter equivalent
    std::map<int, int> size_counter;
    for (const auto& solution : solutions.solutions) {
        size_counter[solution.second.stable_nodes.size()]++;
    }
    std::cout << "Counter: ";
    for (const auto& [size, count] : size_counter) {
        std::cout << size << ":" << count << " ";
    }
    std::cout << std::endl;

    return solutions;
}



