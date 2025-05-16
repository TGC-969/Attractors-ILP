#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <memory>
#include "BooleanNetwork.h"
#include "SolutionObjects.h"
#include "Reachability.cpp"

using namespace std;

// Helper function to simulate numpy's unique
template<typename T>
vector<T> vector_unique(const vector<T>& input) {
    set<T> s(input.begin(), input.end());
    return vector<T>(s.begin(), s.end());
}



    vector<vector<int>> get_included_external(SolutionObjects& solutions, int external_id, int bigger_external_id) {
        auto& external_list = solutions.external_assignments[external_id];
        auto& bigger_external_list = solutions.external_assignments[bigger_external_id];
        const int external_size = external_list[0].size();

        vector<int> not_null_externals;
        for (size_t i = 0; i < bigger_external_list[0].size(); ++i) {
            if (bigger_external_list[0][i] != -1) {
                not_null_externals.push_back(i);
            }
        }

        vector<vector<int>> external_full_assignment = external_list;
        for (int i : not_null_externals) {
            if (external_list[0][i] == -1) {
                vector<vector<int>> new_assignment;
                for (auto& row : external_full_assignment) {
                    vector<int> row0 = row;
                    row0[i] = 0;
                    vector<int> row1 = row;
                    row1[i] = 1;
                    new_assignment.push_back(row0);
                    new_assignment.push_back(row1);
                }
                external_full_assignment = move(new_assignment);
            }
        }

        set<vector<int>> unique_external_1;
        map<vector<int>, vector<int>> inverse_map;
        for (size_t i = 0; i < external_full_assignment.size(); ++i) {
            vector<int> key;
            for (int idx : not_null_externals) {
                key.push_back(external_full_assignment[i][idx]);
            }
            unique_external_1.insert(key);
            inverse_map[key].push_back(i);
        }

        vector<vector<int>> result;
        for (auto& be : bigger_external_list) {
            vector<int> key;
            for (int idx : not_null_externals) {
                key.push_back(be[idx]);
            }
            if (unique_external_1.count(key)) {
                for (int idx : inverse_map[key]) {
                    result.push_back(external_full_assignment[idx]);
                }
            }
        }

        return result;
    }

    void build_solutions_hierarchy_tree(BooleanNetwork& network, SolutionObjects& solutions) {
        map<int, vector<int>> edges_list;
        map<pair<int, int>, vector<vector<int>>> included_externals;

        vector<TrapSpace> ordered_solution;
        for (auto& [id, sol] : solutions.solutions) {
            ordered_solution.push_back(sol);
        }
        sort(ordered_solution.begin(), ordered_solution.end(),
            [](const TrapSpace& a, const TrapSpace& b) {
                return a.stable_nodes.size() < b.stable_nodes.size();
            });

        for (size_t i = 0; i < ordered_solution.size(); ++i) {
            auto& solution = ordered_solution[i];
            if (solution.stable_nodes.size() == network.state_size) break;

            int external_id = solutions.solution_to_externals[solution.solution_id];
            
            for (size_t j = i + 1; j < ordered_solution.size(); ++j) {
                auto& bigger_solution = ordered_solution[j];
                if (bigger_solution.stable_nodes.size() == solution.stable_nodes.size()) continue;

                bool all_included = true;
                for (auto& [k, v] : solution.stable_nodes) {
                    if (bigger_solution.stable_nodes[k] != v) {
                        all_included = false;
                        break;
                    }
                }

                if (all_included) {
                    int bigger_external_id = solutions.solution_to_externals[bigger_solution.solution_id];
                    if (external_id == bigger_external_id) {
                        edges_list[solution.solution_id].push_back(bigger_solution.solution_id);
                        continue;
                    }

                    auto key = make_pair(external_id, bigger_external_id);
                    if (!included_externals.count(key)) {
                        included_externals[key] = get_included_external(solutions, external_id, bigger_external_id);
                    }

                    if (!included_externals[key].empty()) {
                        edges_list[solution.solution_id].push_back(bigger_solution.solution_id);
                    }
                }
            }
        }

        solutions.all_included_solutions = edges_list;
        solutions.all_included_externals = included_externals;
    }

void check_if_included(BooleanNetwork& network, SolutionObjects& solutions,  int solution_id, const vector<int>& included_ids,
                          const vector<int>& externals, map<string, int>& done,
                          bool is_verify_sub_solutions) {
    if (!is_verify_sub_solutions) {
        solutions.solutions[solution_id].mark_as_included_solution();
        return;
    }

    vector<TrapSpace> included_solutions;
    for (int id : included_ids) {
        included_solutions.push_back(solutions.solutions[id]);
    }

    auto& stable_nodes = solutions.solutions[solution_id].stable_nodes;
    auto exploration_functions = get_reduced_threshold_functions(
        network, stable_nodes, externals, included_solutions);

    string key;
    for (auto& [k, v] : exploration_functions) {
        key += to_string(k) + ":";
        for (auto& [f, t] : v) {
            key += "(";
            for (auto& [i, w] : f) key += to_string(i) + ",";
            key += ")" + to_string(t);
        }
    }

    if (done.count(key)) {
        if (done[key] <= 0) {
            solutions.solutions[solution_id].mark_as_included_solution();
        }
        return;
    }

    int res = check_if_reachable(solutions.solutions[solution_id],
                                             included_solutions, exploration_functions);
    done[key] = res;

    if (res <= 0) {
        solutions.solutions[solution_id].mark_as_included_solution();
    }
}

    void remove_included_solutions(BooleanNetwork& network, SolutionObjects& solutions, bool is_verify_sub_solutions) {
        int original_solution_size = solutions.solutions.size();
        build_solutions_hierarchy_tree(network, solutions);

        map<string, int> done;

        for (auto& [solution_id, included_ids] : solutions.all_included_solutions) {
            if (included_ids.empty()) continue;

            int external_id = solutions.solution_to_externals[solution_id];
            auto& solution = solutions.solutions[solution_id];

            vector<int> not_stable_state;
            for (int i = 0; i < network.state_size; ++i) {
                if (!solution.stable_nodes.count(i)) {
                    not_stable_state.push_back(i);
                }
            }

            set<int> not_empty_externals;
            for (int i : not_stable_state) {
                for (auto& [f, t] : network.threshold_functions[i]) {
                    for (auto& [k, v] : f) {
                        if (k >= network.state_size) {
                            not_empty_externals.insert(k - network.state_size);
                        }
                    }
                }
            }

            vector<int> not_empty_ext_vec(not_empty_externals.begin(), not_empty_externals.end());
            sort(not_empty_ext_vec.begin(), not_empty_ext_vec.end());

            auto& external_list = solutions.external_assignments[external_id];

            if (not_empty_ext_vec.empty()) {
                check_if_included(network, solutions, solution_id, included_ids, {}, done, is_verify_sub_solutions);
                continue;
            }

            map<vector<int>, vector<int>> unique_externals;
            for (size_t i = 0; i < external_list.size(); ++i) {
                vector<int> key;
                for (int ext : not_empty_ext_vec) {
                    key.push_back(external_list[i][ext]);
                }
                unique_externals[key].push_back(i);
            }

            vector<vector<int>> not_included_externals;
            for (auto& [key, indices] : unique_externals) {
                bool found = false;
                for (int s_id : included_ids) {
                    auto map_key = make_pair(s_id, solution_id);
                    if (!solutions.all_included_externals.count(map_key)) continue;

                    auto& included_external = solutions.all_included_externals[map_key];
                    for (auto& ext : included_external) {
                        vector<int> ext_key;
                        for (int idx : not_empty_ext_vec) {
                            ext_key.push_back(ext[idx]);
                        }
                        if (ext_key == key) {
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }

                if (!found) {
                    for (int idx : indices) {
                        not_included_externals.push_back(external_list[idx]);
                    }
                } else {
                    check_if_included(network, solutions, solution_id, included_ids, key, done, is_verify_sub_solutions);
                }
            }

            if (!not_included_externals.empty() && not_included_externals.size() < external_list.size()) {
                // Create new solution
                int new_sol_id = solutions.add_solution(solution.stable_nodes);
                int new_ext_id = solutions.add_externals_assignments(not_included_externals);
                solutions.update_external_to_solution(new_sol_id, new_ext_id);
                cout << "New solution: " << solution_id << " ----> " << new_sol_id << endl;
            }
        }

        // Remove marked solutions
        vector<int> to_remove;
        for (auto& [id, sol] : solutions.solutions) {
            if (sol.included_solution) {
                to_remove.push_back(id);
            }
        }
        for (int id : to_remove) {
            solutions.remove_solution(id);
        }

        cout << "Num of solutions: " << solutions.solutions.size() << " / " << original_solution_size << endl;
    }

